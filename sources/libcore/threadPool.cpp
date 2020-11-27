#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>
#include <cage-core/logger.h>

#include <vector>
#include <exception>

namespace cage
{
	namespace
	{
		class ThreadPoolImpl : public ThreadPool
		{
		public:
			Holder<Barrier> barrier1;
			Holder<Barrier> barrier2;
			Holder<Mutex> mutex = newMutex();
			std::exception_ptr exptr;
			std::vector<Holder<Thread>> thrs;
			uint32 threadIndexInitializer = 0;
			const uint32 threadsCount = 0;
			bool ending = false;

			explicit ThreadPoolImpl(const string &threadNames, uint32 threads) : threadsCount(threads == m ? processorsCount() : threads)
			{
				barrier1 = newBarrier(threadsCount + 1);
				barrier2 = newBarrier(threadsCount + 1);
				thrs.resize(threadsCount);
				for (uint32 i = 0; i < threadsCount; i++)
					thrs[i] = newThread(Delegate<void()>().bind<ThreadPoolImpl, &ThreadPoolImpl::threadEntryLocal>(this), stringizer() + threadNames + i);
			}

			~ThreadPoolImpl()
			{
				ending = true;
				{ ScopeLock l(barrier1); }
				thrs.clear(); // all threads must be destroyed before any of the barriers are
			}

			void threadEntryLocal()
			{
				uint32 thrIndex = m;
				{
					ScopeLock l(mutex);
					thrIndex = threadIndexInitializer++;
				}
				while (true)
				{
					{ ScopeLock l(barrier1); }
					if (ending)
						break;
					try
					{
						function(thrIndex, threadsCount);
					}
					catch (...)
					{
						ScopeLock l(mutex);
						if (exptr)
						{
							CAGE_LOG(SeverityEnum::Warning, "thread", "discarding an exception caught in a thread pool");
							detail::logCurrentCaughtException();
						}
						else
							exptr = std::current_exception();
					}
					{ ScopeLock l(barrier2); }
				}
			}

			void run()
			{
				if (threadsCount == 0)
				{
					function(0, 1);
					return;
				}

				{ ScopeLock l(barrier1); }
				{ ScopeLock l(barrier2); }
				if (exptr)
					std::rethrow_exception(exptr);
			}
		};
	}

	uint32 ThreadPool::threadsCount() const
	{
		const ThreadPoolImpl *impl = (const ThreadPoolImpl *)this;
		return impl->threadsCount;
	}

	void ThreadPool::run()
	{
		ThreadPoolImpl *impl = (ThreadPoolImpl *)this;
		impl->run();
	}

	Holder<ThreadPool> newThreadPool(const string &threadNames, uint32 threadsCount)
	{
		return detail::systemArena().createImpl<ThreadPool, ThreadPoolImpl>(threadNames, threadsCount);
	}

	void threadPoolTasksSplit(uint32 threadIndex, uint32 threadsCount, uint32 tasksCount, uint32 &begin, uint32 &end)
	{
		if (threadsCount == 0)
		{
			CAGE_ASSERT(threadIndex == 0);
			begin = 0;
			end = tasksCount;
			return;
		}
		CAGE_ASSERT(threadIndex < threadsCount);
		uint32 tasksPerThread = tasksCount / threadsCount;
		begin = threadIndex * tasksPerThread;
		end = begin + tasksPerThread;
		uint32 remaining = tasksCount - threadsCount * tasksPerThread;
		uint32 off = threadsCount - remaining;
		if (threadIndex >= off)
		{
			begin += threadIndex - off;
			end += threadIndex - off + 1;
		}
	}
}
