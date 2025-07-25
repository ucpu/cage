#include <atomic>
#include <exception>
#include <vector>

#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/debug.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/tasks.h>

namespace cage
{
	namespace
	{
		std::atomic<uint64> globalNextTaskId = 1;
		static_assert(std::atomic<uint64>::is_always_lock_free);

		struct ThreadData
		{
			uint64 currentTaskId = 0;
			bool taskingThread = false;
		};

		thread_local ThreadData threadData;

		Mutex *exceptionsMutex()
		{
			static Holder<Mutex> *mut = new Holder<Mutex>(newMutex()); // this leak is intentional
			return +*mut;
		}

		struct TaskImpl : public AsyncTask
		{
			const privat::TaskCreateConfig config;
			const uint64 parentTaskId = threadData.currentTaskId;
			const uint64 myTaskId = globalNextTaskId.fetch_add(1, std::memory_order_relaxed);
			std::exception_ptr storedException;
			std::atomic<uint32> enqueued = 0; // number of times pushed into the queue
			std::atomic<uint32> executing = 0; // number of times the task started executing (this is the next invocation index)
			std::atomic<uint32> finished = 0; // number of times the execution has successfully finished
			std::atomic<bool> completed = false; // all invocations finished, or exception was thrown, or the task was aborted

			TaskImpl(privat::TaskCreateConfig &&config) : config(std::move(config))
			{
				if (config.invocations == 0)
					completed = true;
			}

			~TaskImpl()
			{
				try
				{
					CAGE_ASSERT(completed && "task must complete before destructor. did you forgot to wait on it?");
				}
				catch (...)
				{
					CAGE_LOG_THROW(Stringizer() + config.name);
					detail::logCurrentCaughtException();
					detail::irrecoverableError("exception thrown in ~TaskImpl");
				}
			}

			void execute()
			{
				if (completed)
					return;
				ProfilingScope profiling(config.name);
				const uint32 idx = executing++;
				CAGE_ASSERT(idx < config.invocations);
				profiling.set(Stringizer() + "task invocation: " + idx + " / " + config.invocations);
				try
				{
					uint64 prevTaskId = myTaskId;
					std::swap(prevTaskId, threadData.currentTaskId);
					const ScopeGuard scopeGuard([&] { std::swap(prevTaskId, threadData.currentTaskId); });

					config.runner(config, idx);
				}
				catch (...)
				{
					{
						ScopeLock lock(exceptionsMutex());
						storedException = std::current_exception();
					}
					complete();
					return;
				}
				if (++finished == config.invocations)
					complete();
			}

			bool done() const { return completed; }

			void wait();

			void complete()
			{
				enqueued = config.invocations; // avoid any further enqueues
				completed = true;
				completed.notify_all();
			}

			void abort()
			{
				try
				{
					throw Exception(std::source_location(), SeverityEnum::Info, "task aborted");
				}
				catch (...)
				{
					ScopeLock lock(exceptionsMutex());
					storedException = std::current_exception();
				}
				complete();
			}
		};

		struct TasksQueue : public ConcurrentQueue<Holder<TaskImpl>>
		{
			bool tryPopFilter(Holder<TaskImpl> &value)
			{
				const uint64 filterId = threadData.currentTaskId;
				ScopeLock sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentQueueTerminated, "tasks concurrent queue terminated");
				for (auto it = items.begin(); it != items.end(); it++)
				{
					if ((*it)->parentTaskId != filterId)
						continue; // prevent the parent task from being blocked by potentially long-lasting unrelated task
					value = std::move(*it);
					items.erase(it);
					writer->signal();
					return true;
				}
				return false;
			}
		};

		TasksQueue &queue()
		{
			static TasksQueue *q = new TasksQueue();
			return *q;
		}

		void enqueue(Holder<TaskImpl> &&tsk)
		{
			CAGE_ASSERT(tsk);
			if (tsk->enqueued++ < tsk->config.invocations)
				queue().push(std::move(tsk));
		}

		void TaskImpl::wait()
		{
			ProfilingScope profiling("waiting for task");
			profiling.set(String(config.name));
			if (threadData.taskingThread)
			{
				TasksQueue &q = queue();
				while (!completed)
				{
					Holder<TaskImpl> t;
					if (q.tryPopFilter(t))
					{
						enqueue(t.share());
						t->execute();
					}
					else
						threadYield();
				}
			}
			else
			{
				while (!completed)
					completed.wait(false);
			}

			CAGE_ASSERT(completed);
			ScopeLock lock(exceptionsMutex());
			if (storedException)
				std::rethrow_exception(storedException);
			else
			{
				CAGE_ASSERT(enqueued >= config.invocations);
				CAGE_ASSERT(executing == config.invocations);
				CAGE_ASSERT(finished == config.invocations);
			}
		}

		struct Executor : private Immovable
		{
			std::vector<Holder<Thread>> threads;

			Executor()
			{
				threads.resize(processorsCount());
				uint32 index = 0;
				for (auto &it : threads)
				{
					// using ~ in the name to move the threads to bottom in the profiler
					it = newThread(&threadEntry, Stringizer() + "~ tasks " + index++);
				}
			}

			~Executor()
			{
				queue().terminate();
				threads.clear();
			}

			static void threadEntry()
			{
				threadData.taskingThread = true;
				try
				{
					TasksQueue &q = queue();
					while (true)
					{
						Holder<TaskImpl> t;
						q.pop(t);
						enqueue(t.share());
						t->execute();
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// nothing
				}
			}
		};

		void executorInitialization()
		{
			static Executor executor;
		}
	}

	bool AsyncTask::done() const
	{
		const TaskImpl *impl = (const TaskImpl *)this;
		return impl->done();
	}

	void AsyncTask::wait()
	{
		TaskImpl *impl = (TaskImpl *)this;
		impl->wait();
	}

	void AsyncTask::abort()
	{
		TaskImpl *impl = (TaskImpl *)this;
		impl->abort();
	}

	namespace privat
	{
		void tasksRunBlocking(TaskCreateConfig &&task)
		{
			// stack-allocated object would be destroyed immediately after an exception escapes from the wait function,
			// but it may still be enqueued for more invocations
			// therefore an actual holder is required
			auto t = tasksRunAsync(std::move(task));
			t->wait();
		}

		Holder<AsyncTask> tasksRunAsync(TaskCreateConfig &&task)
		{
			executorInitialization();
			Holder<TaskImpl> impl = systemMemory().createHolder<TaskImpl>(std::move(task));
			enqueue(impl.share());
			return std::move(impl).cast<AsyncTask>();
		}

		template<bool Async>
		auto tasksRunImpl(StringPointer name, Delegate<void(uint32)> function, uint32 invocations)
		{
			static_assert(sizeof(TaskCreateConfig::function) == sizeof(function));
			TaskCreateConfig tsk;
			tsk.name = name;
			tsk.function = reinterpret_cast<Delegate<void()> &>(function);
			tsk.runner = +[](const TaskCreateConfig &task, uint32 idx)
			{
				const Delegate<void(uint32)> function = reinterpret_cast<const Delegate<void(uint32)> &>(task.function);
				function(idx);
			};
			tsk.invocations = invocations;
			return tasksRunImpl<Async>(std::move(tsk));
		}
	}

	void tasksRunBlocking(StringPointer name, Delegate<void(uint32)> function, uint32 invocations)
	{
		return privat::tasksRunImpl<false>(name, function, invocations);
	}

	Holder<AsyncTask> tasksRunAsync(StringPointer name, Delegate<void(uint32)> function, uint32 invocations)
	{
		return privat::tasksRunImpl<true>(name, function, invocations);
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
