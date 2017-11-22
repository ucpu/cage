#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <pthread.h>
#endif

namespace cage
{
	namespace
	{
		class barrierImpl : public barrierClass
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS

			holder<mutexClass> mut;
			holder<semaphoreClass> sem1, sem2;
			uint32 total, current;

			barrierImpl(uint32 value) : mut(newMutex()), sem1(newSemaphore(0, value)), sem2(newSemaphore(0, value)), total(value), current(0)
			{}

			~barrierImpl()
			{}

			void lock()
			{
				{
					scopeLock<mutexClass> l(mut);
					if (++current == total)
					{
						for (uint32 i = 0; i < total; i++)
							sem1->unlock();
					}
				}
				sem1->lock();
				{
					scopeLock<mutexClass> l(mut);
					if (--current == 0)
					{
						for (uint32 i = 0; i < total; i++)
							sem2->unlock();
					}
				}
				sem2->lock();
			}

#else

			pthread_barrier_t bar;

			barrierImpl(uint32 value)
			{
				pthread_barrier_init(&bar, NULL, value);
			}

			~barrierImpl()
			{
				pthread_barrier_destroy(&bar);
			}

			void lock()
			{
				pthread_barrier_wait(&bar);
			}

#endif
		};
	}

	void barrierClass::lock()
	{
		barrierImpl *impl = (barrierImpl*)this;
		impl->lock();;
	}

	void barrierClass::unlock()
	{
		// does nothing
	}

	holder<barrierClass> newBarrier(uint32 value)
	{
		return detail::systemArena().createImpl<barrierClass, barrierImpl>(value);
	}
}
