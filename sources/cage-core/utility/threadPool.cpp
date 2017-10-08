#include <vector>
#include <exception>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadPool.h>

namespace cage
{
	namespace
	{
		class threadPoolImpl : public threadPoolClass
		{
		public:
			std::vector<holder<threadClass>> thrs;
			holder<barrierClass> barrier1;
			holder<barrierClass> barrier2;
			holder<mutexClass> mutex;
			std::exception_ptr exptr;
			uint32 threadIndexInitializer, threadsCount;
			volatile bool ending;

			threadPoolImpl(const string threadNames, uint32 threads) : threadIndexInitializer(0), threadsCount(threads ? threads : processorsCount()), ending(false)
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
				uint32 thrIndex = -1;
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
						if (!exptr)
							exptr = std::current_exception();
					}
					{ scopeLock<barrierClass> l(barrier2); }
				}
			}

			void run()
			{
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

	holder<threadPoolClass> newThreadPool(const string threadNames, uint32 threadsCount)
	{
		return detail::systemArena().createImpl<threadPoolClass, threadPoolImpl>(threadNames, threadsCount);
	}
}