#include <atomic>
#include <exception>
#include <mutex>
#include <vector>

#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/profiling.h>
#include <cage-core/tasks.h>

namespace cage
{
	namespace
	{
		struct ThreadData
		{
			sint32 currentPriority = std::numeric_limits<sint32>().min();
			bool runningTask = false;
			bool taskingThread = false;
		};

		thread_local ThreadData threadData;

		struct PrioritySwapper : private Immovable
		{
			sint32 origPriority = m;
			bool origTask = true;

			explicit PrioritySwapper(sint32 taskPriority) : origPriority(taskPriority)
			{
				ThreadData &thr = threadData;
				std::swap(thr.currentPriority, origPriority);
				std::swap(thr.runningTask, origTask);
			}

			~PrioritySwapper()
			{
				ThreadData &thr = threadData;
				thr.currentPriority = origPriority;
				thr.runningTask = origTask;
			}
		};

		struct TaskImpl : public AsyncTask
		{
			const privat::TaskCreateConfig config;
			std::mutex exceptionMutex;
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

			void execute()
			{
				if (completed)
					return;
				ProfilingScope profiling(config.name);
				PrioritySwapper prioritySwapper(config.priority);
				const uint32 idx = executing++;
				CAGE_ASSERT(idx < config.invocations);
				profiling.set(Stringizer() + "task invocation: " + idx + " / " + config.invocations + ", priority: " + config.priority);
				try
				{
					config.runner(config, idx);
				}
				catch (...)
				{
					{
						std::scoped_lock lock(exceptionMutex);
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
					std::scoped_lock lock(exceptionMutex);
					storedException = std::current_exception();
				}
				complete();
			}
		};

		struct TasksQueue : public ConcurrentQueue<Holder<TaskImpl>>
		{
			bool tryPopFilter(Holder<TaskImpl> &value, sint32 requiredPriority)
			{
				ScopeLock sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
				for (auto it = items.begin(); it != items.end(); it++)
				{
					if ((*it)->config.priority >= requiredPriority)
					{
						value = std::move(*it);
						items.erase(it);
						writer->signal();
						return true;
					}
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
					if (q.tryPopFilter(t, config.priority)) // allow same priority
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
			std::scoped_lock lock(exceptionMutex);
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
		auto tasksRunImpl(StringPointer name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
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
			tsk.priority = priority;
			return tasksRunImpl<Async>(std::move(tsk));
		}
	}

	void tasksYield()
	{
		const sint32 requiredPriority = threadData.currentPriority + 1; // requires strictly higher priority
		while (true)
		{
			Holder<TaskImpl> t;
			if (!queue().tryPopFilter(t, requiredPriority))
				break;
			enqueue(t.share());
			t->execute();
		}
	}

	sint32 tasksCurrentPriority()
	{
		return threadData.runningTask ? threadData.currentPriority : 0;
	}

	void tasksRunBlocking(StringPointer name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		return privat::tasksRunImpl<false>(name, function, invocations, priority);
	}

	Holder<AsyncTask> tasksRunAsync(StringPointer name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority)
	{
		return privat::tasksRunImpl<true>(name, function, invocations, priority);
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
