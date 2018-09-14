#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <pthread.h>
#endif

#ifdef CAGE_SYSTEM_MAC

#include <cerrno>

namespace
{
	typedef int pthread_barrierattr_t;

	typedef struct
	{
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		int count;
		int tripCount;
	} pthread_barrier_t;

	int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
	{
		if (count == 0)
		{
			errno = EINVAL;
			return -1;
		}
		if (pthread_mutex_init(&barrier->mutex, 0) < 0)
		{
			return -1;
		}
		if (pthread_cond_init(&barrier->cond, 0) < 0)
		{
			pthread_mutex_destroy(&barrier->mutex);
			return -1;
		}
		barrier->tripCount = count;
		barrier->count = 0;
		return 0;
	}

	int pthread_barrier_destroy(pthread_barrier_t *barrier)
	{
		pthread_cond_destroy(&barrier->cond);
		pthread_mutex_destroy(&barrier->mutex);
		return 0;
	}

	int pthread_barrier_wait(pthread_barrier_t *barrier)
	{
		pthread_mutex_lock(&barrier->mutex);
		++(barrier->count);
		if (barrier->count >= barrier->tripCount)
		{
			barrier->count = 0;
			pthread_cond_broadcast(&barrier->cond);
			pthread_mutex_unlock(&barrier->mutex);
			return 1;
		}
		else
		{
			pthread_cond_wait(&barrier->cond, &(barrier->mutex));
			pthread_mutex_unlock(&barrier->mutex);
			return 0;
		}
	}
}

#endif // CAGE_SYSTEM_MAC

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

