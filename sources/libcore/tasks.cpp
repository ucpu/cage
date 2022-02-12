#include <cage-core/tasks.h>
#include <cage-core/debug.h>
#include <cage-core/concurrent.h>
#include <cage-core/profiling.h>

#include <exception>
#include <vector>
#include <queue>
#include <atomic>
#include <condition_variable>

namespace cage
{
	namespace
	{
		struct TaskImpl;

		struct ConcurrentPriorityQueueTerminated : public Exception
		{
			using Exception::Exception;
		};

		class ConcurrentPriorityQueue : private Immovable
		{
		public:
			using Value = Holder<TaskImpl>;

			CAGE_FORCE_INLINE void push(Value &&value)
			{
				ScopeLock sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentPriorityQueueTerminated, "concurrent queue terminated");
				items.emplace(std::move(value));
				cond->signal();
			}

			CAGE_FORCE_INLINE void pop(Value &value)
			{
				ScopeLock sl(mut);
				while (true)
				{
					if (stop)
						CAGE_THROW_SILENT(ConcurrentPriorityQueueTerminated, "concurrent queue terminated");
					if (items.empty())
						cond->wait(sl);
					else
					{
						value = std::move(const_cast<Value &>(items.top()));
						items.pop();
						return;
					}
				}
			}

			CAGE_FORCE_INLINE bool tryPop(Value &value, const sint32 requiredPriority)
			{
				ScopeLock sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentPriorityQueueTerminated, "concurrent queue terminated");
				if (!items.empty())
				{
					const Value &top = items.top();
					if (!valid(top, requiredPriority))
						return false;
					value = std::move(const_cast<Value &>(top));
					items.pop();
					return true;
				}
				return false;
			}

			void terminate()
			{
				{
					ScopeLock sl(mut); // mandate memory barriers
					stop = true;
				}
				cond->broadcast();
			}

		private:
			struct Comparator
			{
				bool operator() (const Value &a, const Value &b) const noexcept;
			};

			bool valid(const Value &value, const sint32 requiredPriority) const noexcept;

			Holder<Mutex> mut = newMutex();
			Holder<ConditionalVariableBase> cond = newConditionalVariableBase();
			std::priority_queue<Value, std::vector<Value>, Comparator> items;
			bool stop = false;
		};

		struct ThrData
		{
			sint32 currentPriority = std::numeric_limits<sint32>().min();
			bool executorThread = false;
		};

		thread_local ThrData thrData;

		struct Executor : private Immovable
		{
			Executor()
			{
				threads.resize(processorsCount());
				uint32 index = 0;
				for (auto &t : threads)
					t = newThread(Delegate<void()>().bind<Executor, &Executor::threadEntry>(this), Stringizer() + "tasks exec " + index++);
			}

			~Executor()
			{
				queue.terminate();
				threads.clear();
			}

			void schedule(const Holder<TaskImpl> &tsk);
			bool tryRun(sint32 requiredPriority);
			void run();

		private:
			void threadEntry()
			{
				profilingThreadOrder(1000);
				thrData.executorThread = true;
				try
				{
					while (true)
						run();
				}
				catch (const ConcurrentPriorityQueueTerminated &)
				{
					// nothing
				}
			}

			ConcurrentPriorityQueue queue;
			std::vector<Holder<Thread>> threads;
		};

		Executor &executor()
		{
			static Executor exec;
			return exec;
		}

		struct ThreadPriorityUpdater : private Immovable
		{
			sint32 &thrPrio = thrData.currentPriority;
			sint32 tmpPrio = m;

			CAGE_FORCE_INLINE ThreadPriorityUpdater(sint32 tskPrio) : tmpPrio(tskPrio)
			{
				CAGE_ASSERT(tskPrio >= thrPrio);
				std::swap(tmpPrio, thrPrio);
			}

			CAGE_FORCE_INLINE ~ThreadPriorityUpdater()
			{
				thrPrio = tmpPrio;
			}
		};

		struct TaskImpl : public AsyncTask
		{
			const privat::TaskRunnerConfig runnerConfig;
			const privat::TaskRunner runner = nullptr;
			const StringLiteral name;
			const sint32 priority = 0;
			const uint32 invocations = 0;
			std::atomic<uint32> scheduled = 0;
			std::atomic<uint32> executing = 0;
			std::atomic<uint32> finished = 0;
			std::atomic<bool> done = false;
			std::mutex mutex = {}; // using std mutex etc to avoid dynamic allocation associated with cage::Mutex
			std::condition_variable cond = {};
			std::exception_ptr exptr = nullptr;

			CAGE_FORCE_INLINE explicit TaskImpl(privat::TaskCreateConfig &&task) : runnerConfig(copy(std::move(task.data), task.function, task.elements)), runner(task.runner), name(task.name), priority(task.priority), invocations(task.invocations)
			{
				if (invocations == 0)
					done = true;
			}

			CAGE_FORCE_INLINE ~TaskImpl()
			{
				CAGE_ASSERT(done);
			}

			CAGE_FORCE_INLINE void wait()
			{
				if (thrData.executorThread)
				{
					auto &exec = executor();
					while (!done)
					{
						while (exec.tryRun(priority) && !done);
						if (!done)
							threadYield();
					}
				}
				else
				{
					std::unique_lock lock(mutex);
					while (!done)
						cond.wait(lock);
				}
				rethrow();
			}

			CAGE_FORCE_INLINE void rethrow()
			{
				CAGE_ASSERT(done);
				if (exptr)
				{
					std::unique_lock lock(mutex);
					std::exception_ptr tmp = nullptr;
					std::swap(tmp, exptr);
					std::rethrow_exception(tmp);
				}
			}

			CAGE_FORCE_INLINE void execute() noexcept
			{
				try
				{
					const uint32 idx = executing++;
					{
						ThreadPriorityUpdater prio(priority);
						ProfilingScope prof(String(name), "tasks");
						runner(runnerConfig, idx);
					}
					if (++finished == invocations)
					{
						{
							std::unique_lock lck(mutex);
							done = true;
						}
						cond.notify_all();
					}
				}
				catch (...)
				{
					{
						std::unique_lock lck(mutex);
						exptr = std::current_exception();
						done = true;
					}
					cond.notify_all();
				}
			}

		private:
			CAGE_FORCE_INLINE static privat::TaskRunnerConfig copy(Holder<void> &&data, Delegate<void()> function, uint32 elements)
			{
				privat::TaskRunnerConfig res;
				res.data = std::move(data);
				res.function = function;
				res.elements = elements;
				return res;
			}
		};

		CAGE_FORCE_INLINE bool ConcurrentPriorityQueue::Comparator::operator() (const Value &a, const Value &b) const noexcept
		{
			return a->priority < b->priority;
		}

		CAGE_FORCE_INLINE bool ConcurrentPriorityQueue::valid(const Value &value, const sint32 requiredPriority) const noexcept
		{
			return value->priority >= requiredPriority;
		}

		CAGE_FORCE_INLINE void Executor::schedule(const Holder<TaskImpl> &tsk)
		{
			if (tsk->scheduled++ < tsk->invocations)
				queue.push(tsk.share());
		}

		CAGE_FORCE_INLINE bool Executor::tryRun(sint32 requiredPriority)
		{
			Holder<TaskImpl> tsk;
			if (queue.tryPop(tsk, requiredPriority))
			{
				schedule(tsk);
				tsk->execute();
				return true;
			}
			return false;
		}

		CAGE_FORCE_INLINE void Executor::run()
		{
			Holder<TaskImpl> tsk;
			queue.pop(tsk);
			schedule(tsk);
			tsk->execute();
		}
	}

	bool AsyncTask::done() const
	{
		const TaskImpl *impl = (const TaskImpl *)this;
		return impl->done;
	}

	void AsyncTask::wait()
	{
		TaskImpl *impl = (TaskImpl *)this;
		impl->wait();
	}

	namespace privat
	{
		void tasksRunBlocking(TaskCreateConfig &&task)
		{
			tasksRunAsync(std::move(task))->wait();
		}

		Holder<AsyncTask> tasksRunAsync(TaskCreateConfig &&task)
		{
			Holder<TaskImpl> impl = systemMemory().createHolder<TaskImpl>(std::move(task));
			if (!impl->done)
				executor().schedule(impl.share());
			return std::move(impl).cast<AsyncTask>();
		}
	}

	namespace
	{
		template<bool Async>
		auto tasksRunImpl(StringLiteral name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
		{
			static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
			privat::TaskCreateConfig tsk;
			tsk.name = name;
			tsk.function = *(Delegate<void()> *) & function;
			tsk.runner = +[](const privat::TaskRunnerConfig &task, uint32 idx) {
				Delegate<void(uint32)> function = *(Delegate<void(uint32)> *) & task.function;
				function(idx);
			};
			tsk.invocations = invocations;
			tsk.priority = priority;
			return privat::tasksRunImpl<Async>(std::move(tsk));
		}
	}

	void tasksRunBlocking(StringLiteral name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		return tasksRunImpl<false>(name, function, invocations, priority);
	}

	Holder<AsyncTask> tasksRunAsync(StringLiteral name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		return tasksRunImpl<true>(name, function, invocations, priority);
	}

	void tasksYield()
	{
		auto &exec = executor();
		const sint32 reqPri = thrData.currentPriority + 1; // do not allow same-priority tasks
		while (exec.tryRun(reqPri));
	}

	sint32 tasksCurrentPriority()
	{
		return thrData.currentPriority;
	}

	std::pair<uint32, uint32> tasksSplit(uint32 groupIndex, uint32 groupsCount, uint32 tasksCount)
	{
		if (groupsCount == 0)
		{
			CAGE_ASSERT(groupIndex == 0);
			return { 0, tasksCount };
		}
		CAGE_ASSERT(groupIndex < groupsCount);
		uint32 tasksPerThread = tasksCount / groupsCount;
		uint32 begin = groupIndex * tasksPerThread;
		uint32 end = begin + tasksPerThread;
		uint32 remaining = tasksCount - groupsCount * tasksPerThread;
		uint32 off = groupsCount - remaining;
		if (groupIndex >= off)
		{
			begin += groupIndex - off;
			end += groupIndex - off + 1;
		}
		return { begin, end };
	}
}
