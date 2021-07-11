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
		struct QueueItem : private Noncopyable
		{
			Holder<struct AsyncTaskImpl> impl;
			uint32 idx = m;

			QueueItem(const Holder<struct AsyncTaskImpl> &impl, uint32 idx);
			~QueueItem();

			QueueItem() = default;

			QueueItem(QueueItem &&other) noexcept
			{
				std::swap(impl, other.impl);
				std::swap(idx, other.idx);
			}

			void operator = (QueueItem &&other) noexcept
			{
				std::swap(impl, other.impl);
				std::swap(idx, other.idx);
			}

			void run() noexcept;
		};

		struct ThreadData
		{
			bool taskExecutor = false;
		};

		thread_local ThreadData threadData;

		struct Executor : private Immovable
		{
			ConcurrentQueue<QueueItem> queue;

			Holder<Mutex> pendingMut = newMutex();
			Holder<ConditionalVariableBase> pendingCond = newConditionalVariableBase();

			std::vector<Holder<Thread>> threads;

			void threadEntry()
			{
				threadData.taskExecutor = true;
				try
				{
					while (true)
					{
						QueueItem item;
						queue.pop(item);
						item.run();
					}
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
				queue.terminate();
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

			sint32 pending = 0;

			Holder<Mutex> mut = newMutex(); // protects the exptr
			std::exception_ptr exptr;

			explicit AsyncTaskImpl(privat::AsyncTask &&task_) : task(std::move(task_))
			{}

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

			void init(const Holder<AsyncTaskImpl> &self)
			{
				CAGE_ASSERT(+self == this);
				Executor &exec = executor();
				for (uint32 i = 0; i < task.count; i++)
					exec.queue.push(QueueItem(self, i));
				exec.pendingCond->broadcast(); // there might be available thread(s) that can run the items
			}

			void wait()
			{
				Executor &exec = executor();
				while (true)
				{
					ScopeLock lock(exec.pendingMut);
					if (pending == 0)
						break;
					if (threadData.taskExecutor) // avoid blocking task executor threads with recursive tasks
					{
						QueueItem item;
						if (exec.queue.tryPop(item))
						{
							lock.clear();
							item.run();
							continue;
						}
					}
					exec.pendingCond->wait(lock);
				}

				if (exptr)
				{
					std::exception_ptr tmp;
					std::swap(tmp, exptr);
					std::rethrow_exception(tmp);
				}
			}
		};

		QueueItem::QueueItem(const Holder<struct AsyncTaskImpl> &impl, uint32 idx) : impl(impl.share()), idx(idx)
		{
			ScopeLock lock(executor().pendingMut);
			impl->pending++;
		}

		QueueItem::~QueueItem()
		{
			if (!impl)
				return;
			{
				ScopeLock lock(executor().pendingMut);
				impl->pending--;
			}
			executor().pendingCond->broadcast(); // make sure the thread waiting for this task is notified
		}

		void QueueItem::run() noexcept
		{
			try
			{
				impl->task.runner(impl->task, idx);
			}
			catch (...)
			{
				{
					ScopeLock<Mutex> lock(impl->mut);
					impl->exptr = std::current_exception();
				}
				CAGE_LOG(SeverityEnum::Warning, "tasks", stringizer() + "unhandled exception in a task");
				detail::logCurrentCaughtException();
			}
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
			impl->init(impl);
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
}
