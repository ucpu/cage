#include <cage-core/tasks.h>
#include <cage-core/debug.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>

#include <vector>
#include <atomic>

namespace cage
{
	namespace
	{
		struct ThreadData
		{
			bool taskExecutor = false;
		};

		thread_local ThreadData threadData;

		struct Executor : private Immovable
		{
			ConcurrentQueue<Holder<struct AsyncTaskImpl>> tasks;
			std::vector<Holder<Thread>> threads;

			bool runOne(bool allowWait);

			void threadEntry()
			{
				threadData.taskExecutor = true;
				try
				{
					while (true)
						runOne(true);
				}
				catch (const ConcurrentQueueTerminated &)
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

		struct AsyncTaskImpl : public detail::AsyncTask
		{
			const privat::AsyncTask task;
			Holder<ConditionalVariableBase> cond = newConditionalVariableBase();
			Holder<Mutex> mut = newMutex();
			std::exception_ptr exptr;
			uint32 started = 0;
			uint32 finished = 0;
			bool done = false;

			explicit AsyncTaskImpl(privat::AsyncTask &&task_) : task(std::move(task_))
			{
				if (task.count == 0)
					done = true;
			}

			~AsyncTaskImpl()
			{
				try
				{
					wait();
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Critical, "tasks", stringizer() + "exception thrown in a task was not propagated to the caller (missing call to wait), terminating now");
					detail::logCurrentCaughtException();
					detail::terminate();
				}
			}

			std::pair<uint32, bool> pickNext()
			{
				ScopeLock lck(mut);
				const uint32 idx = started++;
				const bool more = started < task.count;
				return { idx, more };
			}

			void runOne(uint32 idx) noexcept
			{
				try
				{
					CAGE_ASSERT(idx < task.count);
					task.runner(task, idx);
				}
				catch (...)
				{
					{
						ScopeLock lck(mut);
						if (!exptr)
							exptr = std::current_exception();
					}
					CAGE_LOG(SeverityEnum::Warning, "tasks", stringizer() + "unhandled exception in a task");
					detail::logCurrentCaughtException();
				}
				{
					ScopeLock lck(mut);
					finished++;
					if (finished == task.count)
						done = true;
				}
				cond->broadcast();
			}

			void wait()
			{
				Executor &exec = executor();

				while (true)
				{
					{
						ScopeLock lck(mut);
						if (done)
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
							cond->wait(lck);
							continue;
						}
					}
					if (exec.runOne(false))
						continue;
					{
						ScopeLock lck(mut);
						if (done)
						{
							if (exptr)
							{
								std::exception_ptr tmp;
								std::swap(tmp, exptr);
								std::rethrow_exception(tmp);
							}
							return;
						}
						cond->wait(lck);
					}
				}
			}
		};

		bool Executor::runOne(bool allowWait)
		{
			Holder<AsyncTaskImpl> task;
			if (allowWait)
				tasks.pop(task);
			else if (!tasks.tryPop(task))
				return false;
			const auto pick = task->pickNext();
			if (pick.second)
				tasks.push(task.share());
			task->runOne(pick.first);
			return true;
		}
	}

	namespace privat
	{
		void waitTaskRunner(const WaitTask *task, uint32 idx)
		{
			task->runner(*task, idx);
		}

		void tasksRun(const WaitTask &task)
		{
			Delegate<void(uint32)> fnc;
			fnc.bind<const WaitTask *, &waitTaskRunner>(&task);
			cage::tasksRunAsync(fnc, task.count, task.priority)->wait();
		}

		Holder<detail::AsyncTask> tasksRun(AsyncTask &&task)
		{
			Holder<AsyncTaskImpl> impl = systemMemory().createHolder<AsyncTaskImpl>(std::move(task));
			if (impl->task.count > 0)
				executor().tasks.push(impl.share());
			return std::move(impl).cast<detail::AsyncTask>();
		}
	}

	namespace detail
	{
		void AsyncTask::wait()
		{
			AsyncTaskImpl *impl = (AsyncTaskImpl *)this;
			impl->wait();
		}
	}

	void tasksRun(Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		static_assert(sizeof(privat::WaitTask::function) == sizeof(function));
		privat::WaitTask tsk;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::WaitTask &task, uint32 idx) {
			Delegate<void(uint32)> function = *(Delegate<void(uint32)> *)&task.function;
			function(idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		privat::tasksRun(tsk);
	}

	Holder<detail::AsyncTask> tasksRunAsync(Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		static_assert(sizeof(privat::AsyncTask::function) == sizeof(function));
		privat::AsyncTask tsk;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::AsyncTask &task, uint32 idx) {
			Delegate<void(uint32)> function = *(Delegate<void(uint32)> *)&task.function;
			function(idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		return privat::tasksRun(std::move(tsk));
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
