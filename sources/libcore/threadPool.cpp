#include <vector>
#include <exception>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>

namespace cage
{
	namespace
	{
		class threadPoolImpl : public threadPoolClass
		{
		public:
			holder<barrierClass> barrier1;
			holder<barrierClass> barrier2;
			holder<mutexClass> mutex;
			std::exception_ptr exptr;
			std::vector<holder<threadClass>> thrs;
			uint32 threadIndexInitializer, threadsCount;
			bool ending;

			threadPoolImpl(const string &threadNames, uint32 threads) : threadIndexInitializer(0), threadsCount(threads == m ? processorsCount() : threads), ending(false)
			{
				barrier1 = newBarrier(threadsCount + 1);
				barrier2 = newBarrier(threadsCount + 1);
				mutex = newMutex();
				thrs.resize(threadsCount);
				for (uint32 i = 0; i < threadsCount; i++)
					thrs[i] = newThread(delegate<void()>().bind<threadPoolImpl, &threadPoolImpl::threadEntryLocal>(this), threadNames + i);
			}

			~threadPoolImpl()
			{
				ending = true;
				{ scopeLock<barrierClass> l(barrier1); }
				thrs.clear(); // all threads must be destroyed before any of the barriers are
			}

			void threadEntryLocal()
			{
				uint32 thrIndex = m;
				{
					scopeLock<mutexClass> l(mutex);
					thrIndex = threadIndexInitializer++;
				}
				while (true)
				{
					{ scopeLock<barrierClass> l(barrier1); }
					if (ending)
						break;
					try
					{
						function(thrIndex, threadsCount);
					}
					catch (...)
					{
						scopeLock<mutexClass> l(mutex);
						if (exptr)
							CAGE_LOG(severityEnum::Warning, "thread-pool", "discarding an exception caught in a thread pool");
						else
							exptr = std::current_exception();
					}
					{ scopeLock<barrierClass> l(barrier2); }
				}
			}

			void run()
			{
				if (threadsCount == 0)
				{
					function(0, 1);
					return;
				}

				{ scopeLock<barrierClass> l(barrier1); }
				{ scopeLock<barrierClass> l(barrier2); }
				if (exptr)
					std::rethrow_exception(exptr);
			}
		};
	}

	void threadPoolClass::run()
	{
		threadPoolImpl *impl = (threadPoolImpl*)this;
		impl->run();
	}

	holder<threadPoolClass> newThreadPool(const string &threadNames, uint32 threadsCount)
	{
		return detail::systemArena().createImpl<threadPoolClass, threadPoolImpl>(threadNames, threadsCount);
	}
}
