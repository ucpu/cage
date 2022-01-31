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

			void push(Value &&value, sint32 priority)
			{
				ScopeLock sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentPriorityQueueTerminated, "concurrent queue terminated");
				items.emplace(std::move(value), priority);
				cond->signal();
			}

			void pop(Value &value)
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
						value = std::move(const_cast<Item&>(items.top()).first);
						items.pop();
						return;
					}
				}
			}

			bool tryPop(Value &value)
			{
				ScopeLock sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentPriorityQueueTerminated, "concurrent queue terminated");
				if (!items.empty())
				{
					value = std::move(const_cast<Item &>(items.top()).first);
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
			using Item = std::pair<Value, sint32>;

			struct Comparator
			{
				bool operator() (const Item &a, const Item &b) const noexcept
				{
					return a.second < b.second;
				}
			};

			Holder<Mutex> mut = newMutex();
			Holder<ConditionalVariableBase> cond = newConditionalVariableBase();
			std::priority_queue<Item, std::vector<Item>, Comparator> items;
			bool stop = false;
		};

		struct ThrData
		{
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
			void tryRun();
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
			Executor &exec = executor();

			explicit TaskImpl(privat::TaskCreateConfig &&task) : runnerConfig(copy(std::move(task.data), task.function, task.elements)), runner(task.runner), name(task.name), priority(task.priority), invocations(task.invocations)
			{
				if (invocations == 0)
					done = true;
			}

			~TaskImpl()
			{
				CAGE_ASSERT(done);
			}

			void wait()
			{
				if (thrData.executorThread)
				{
					while (!done)
						exec.tryRun();
				}
				else
				{
					std::unique_lock lock(mutex);
					while (!done)
						cond.wait(lock);
				}

				if (exptr)
				{
					std::unique_lock lock(mutex);
					std::exception_ptr tmp = nullptr;
					std::swap(tmp, exptr);
					std::rethrow_exception(tmp);
				}
			}

			void execute() noexcept
			{
				try
				{
					const uint32 idx = executing++;
					ProfilingScope prof(Stringizer() + name + " (" + idx + ")", "tasks");
					runner(runnerConfig, idx);
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
			static privat::TaskRunnerConfig copy(Holder<void> &&data, Delegate<void()> function, uint32 elements)
			{
				privat::TaskRunnerConfig res;
				res.data = std::move(data);
				res.function = function;
				res.elements = elements;
				return res;
			}
		};

		void Executor::schedule(const Holder<TaskImpl> &tsk)
		{
			const auto prio = tsk->priority;
			if (tsk->scheduled++ < tsk->invocations)
				queue.push(tsk.share(), prio);
		}

		void Executor::tryRun()
		{
			Holder<TaskImpl> tsk;
			if (queue.tryPop(tsk))
			{
				schedule(tsk);
				tsk->execute();
			}
			else
				threadYield();
		}

		void Executor::run()
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
		// todo
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
