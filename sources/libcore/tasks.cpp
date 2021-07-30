#include <cage-core/tasks.h>
#include <cage-core/debug.h>
#include <cage-core/concurrent.h>

#include <exception>
#include <vector>
#include <queue>
#include <atomic>

namespace cage
{
	namespace
	{
		struct ConcurrentPriorityQueueTerminated : public Exception
		{
			using Exception::Exception;
		};

		class ConcurrentPriorityQueue : private Immovable
		{
		public:
			using Value = Holder<struct TaskImpl>;

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

		struct ThreadData
		{
			bool taskExecutor = false;
		};

		thread_local ThreadData threadData;

		struct Executor : private Immovable
		{
			ConcurrentPriorityQueue tasks;
			std::vector<Holder<Thread>> threads;
			Holder<ConditionalVariableBase> cond = newConditionalVariableBase();
			Holder<Mutex> mut = newMutex();

			template<bool AllowWait>
			bool runOne();

			void threadEntry()
			{
				threadData.taskExecutor = true;
				try
				{
					while (true)
						runOne<true>();
				}
				catch (const ConcurrentPriorityQueueTerminated &)
				{
					// nothing
				}
			}

			Executor()
			{
				threads.resize(processorsCount());
				uint32 index = 0;
				for (auto &t : threads)
					t = newThread(Delegate<void()>().bind<Executor, &Executor::threadEntry>(this), stringizer() + "tasks exec " + index++);
			}

			~Executor()
			{
				tasks.terminate();
				threads.clear();
			}
		};

		Executor &executor()
		{
			static Executor exec;
			return exec;
		}

		// owned by the proxy AND by the executor queue
		struct TaskImpl : public privat::TaskCreateConfig, private Immovable
		{
			std::exception_ptr exptr;
			std::atomic<uint32> started = 0;
			std::atomic<uint32> finished = 0;
			bool done_ = false;

			explicit TaskImpl(privat::TaskCreateConfig &&task_) : privat::TaskCreateConfig(std::move(task_))
			{
				if (count == 0)
					done_ = true;
			}

			~TaskImpl()
			{
				CAGE_ASSERT(done_);
			}

			std::pair<uint32, bool> pickNext()
			{
				const uint32 idx = started++;
				const bool more = idx + 1 < count;
				return { idx, more };
			}

			void runOne(uint32 idx) noexcept
			{
				try
				{
					CAGE_ASSERT(idx < count);
					runner(*this, idx);
				}
				catch (...)
				{
					{
						ScopeLock lck(executor().mut);
						if (!exptr)
							exptr = std::current_exception();
					}
					CAGE_LOG(SeverityEnum::Warning, "tasks", stringizer() + "unhandled exception in a task");
					detail::logCurrentCaughtException();
				}
				if (++finished == count)
				{
					ScopeLock lck(executor().mut);
					done_ = true;
				}
			}

			bool done() const
			{
				ScopeLock lck(executor().mut);
				return done_;
			}

			void wait()
			{
				Executor &exec = executor();

				while (true)
				{
					{
						ScopeLock lck(executor().mut);
						if (done_)
						{
							if (exptr)
							{
								std::exception_ptr tmp;
								std::swap(tmp, exptr);
								std::rethrow_exception(tmp);
							}
							return;
						}
						if (!threadData.taskExecutor)
						{
							exec.cond->wait(lck);
							continue;
						}
					}
					if (exec.runOne<false>())
						continue;
					{
						ScopeLock lck(executor().mut);
						if (done_)
						{
							if (exptr)
							{
								std::exception_ptr tmp;
								std::swap(tmp, exptr);
								std::rethrow_exception(tmp);
							}
							return;
						}
						exec.cond->wait(lck);
					}
				}

				CAGE_ASSERT(false);
			}
		};

		template<bool AllowWait>
		bool Executor::runOne()
		{
			{
				Holder<TaskImpl> task;
				if constexpr (AllowWait)
					tasks.pop(task);
				else if (!tasks.tryPop(task))
					return false;
				const auto pick = task->pickNext();
				if (pick.second)
					tasks.push(task.share(), task->priority);
				task->runOne(pick.first);
			}
			cond->broadcast();
			return true;
		}

		// owned by the api caller -> it ensures that the wait is always called
		struct TaskProxy : public AsyncTask
		{
			Holder<TaskImpl> impl;

			TaskProxy(Holder<TaskImpl> &&impl) : impl(std::move(impl))
			{}

			~TaskProxy()
			{
				try
				{
					impl->wait();
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Critical, "tasks", stringizer() + "exception thrown in a task was not propagated to the caller (missing call to wait), terminating now");
					detail::logCurrentCaughtException();
					detail::terminate();
				}
			}
		};
	}

	bool AsyncTask::done() const
	{
		const TaskImpl *impl = +((const TaskProxy *)this)->impl;
		return impl->done();
	}

	void AsyncTask::wait()
	{
		TaskImpl *impl = +((TaskProxy *)this)->impl;
		impl->wait();
	}

	namespace privat
	{
		void tasksRunBlocking(TaskCreateConfig &&task)
		{
			if (task.count == 0)
				return;
			TaskImpl impl(std::move(task));
			executor().tasks.push(Holder<TaskImpl>(&impl, nullptr), impl.priority);
			impl.wait();
		}

		Holder<AsyncTask> tasksRunAsync(TaskCreateConfig &&task)
		{
			Holder<TaskProxy> prox = systemMemory().createHolder<TaskProxy>(systemMemory().createHolder<TaskImpl>(std::move(task)));
			if (prox->impl->count > 0)
				executor().tasks.push(prox->impl.share(), prox->impl->priority);
			return std::move(prox).cast<AsyncTask>();
		}
	}

	void tasksRunBlocking(Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
		privat::TaskCreateConfig tsk;
		tsk.function = *(Delegate<void()> *) & function;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Delegate<void(uint32)> function = *(Delegate<void(uint32)> *) & task.function;
			function(idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		privat::tasksRunBlocking(std::move(tsk));
	}

	Holder<AsyncTask> tasksRunAsync(Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
		privat::TaskCreateConfig tsk;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Delegate<void(uint32)> function = *(Delegate<void(uint32)> *)&task.function;
			function(idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		return privat::tasksRunAsync(std::move(tsk));
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
