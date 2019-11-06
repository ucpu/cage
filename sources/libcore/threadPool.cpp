#include <vector>
#include <exception>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>

namespace cage
{
	namespace
	{
		class threadPoolImpl : public threadPool
		{
		public:
			holder<syncBarrier> barrier1;
			holder<syncBarrier> barrier2;
			holder<syncMutex> mutex;
			std::exception_ptr exptr;
			std::vector<holder<threadHandle>> thrs;
			uint32 threadIndexInitializer, threadsCount;
			bool ending;

			threadPoolImpl(const string &threadNames, uint32 threads) : threadIndexInitializer(0), threadsCount(threads == m ? processorsCount() : threads), ending(false)
			{
				barrier1 = newSyncBarrier(threadsCount + 1);
				barrier2 = newSyncBarrier(threadsCount + 1);
				mutex = newSyncMutex();
				thrs.resize(threadsCount);
				for (uint32 i = 0; i < threadsCount; i++)
					thrs[i] = newThread(delegate<void()>().bind<threadPoolImpl, &threadPoolImpl::threadEntryLocal>(this), stringizer() + threadNames + i);
			}

			~threadPoolImpl()
			{
				ending = true;
				{ scopeLock<syncBarrier> l(barrier1); }
				thrs.clear(); // all threads must be destroyed before any of the barriers are
			}

			void threadEntryLocal()
			{
				uint32 thrIndex = m;
				{
					scopeLock<syncMutex> l(mutex);
					thrIndex = threadIndexInitializer++;
				}
				while (true)
				{
					{ scopeLock<syncBarrier> l(barrier1); }
					if (ending)
						break;
					try
					{
						function(thrIndex, threadsCount);
					}
					catch (...)
					{
						scopeLock<syncMutex> l(mutex);
						if (exptr)
							CAGE_LOG(severityEnum::Warning, "threadPool", "discarding an exception caught in a thread pool");
						else
							exptr = std::current_exception();
					}
					{ scopeLock<syncBarrier> l(barrier2); }
				}
			}

			void run()
			{
				if (threadsCount == 0)
				{
					function(0, 1);
					return;
				}

				{ scopeLock<syncBarrier> l(barrier1); }
				{ scopeLock<syncBarrier> l(barrier2); }
				if (exptr)
					std::rethrow_exception(exptr);
			}
		};
	}

	void threadPool::run()
	{
		threadPoolImpl *impl = (threadPoolImpl*)this;
		impl->run();
	}

	holder<threadPool> newThreadPool(const string &threadNames, uint32 threadsCount)
	{
		return detail::systemArena().createImpl<threadPool, threadPoolImpl>(threadNames, threadsCount);
	}

	void threadPoolTasksSplit(uint32 threadIndex, uint32 threadsCount, uint32 tasksCount, uint32 &begin, uint32 &end)
	{
		if (threadsCount == 0)
		{
			begin = 0;
			end = tasksCount;
			return;
		}
		CAGE_ASSERT(threadIndex < threadsCount);
		uint32 tasksPerThread = tasksCount / threadsCount;
		begin = threadIndex * tasksPerThread;
		end = threadIndex + 1 == threadsCount ? tasksCount : begin + tasksPerThread;
	}
}
