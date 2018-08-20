#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#elif defined(CAGE_SYSTEM_MAC)
#include <dispatch/dispatch.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <cerrno>
#endif

namespace cage
{
	namespace
	{
		class semaphoreImpl : public semaphoreClass
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			HANDLE sem;
#elif defined(CAGE_SYSTEM_MAC)
            dispatch_semaphore_t sem;
#else
			sem_t sem;
#endif

			semaphoreImpl(uint32 value, uint32 max)
			{
#ifdef CAGE_SYSTEM_WINDOWS
				sem = CreateSemaphore(nullptr, value, max, nullptr);
#elif defined(CAGE_SYSTEM_MAC)
				sem = dispatch_semaphore_create(value);
#else
				sem_init(&sem, 0, value);
#endif
			}

			~semaphoreImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				CloseHandle(sem);
#elif defined(CAGE_SYSTEM_MAC)
                dispatch_release(sem);
#else
				sem_destroy(&sem);
#endif
			}
		};
	}

	void semaphoreClass::lock()
	{
		semaphoreImpl *impl = (semaphoreImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WaitForSingleObject(impl->sem, INFINITE);
#elif defined(CAGE_SYSTEM_MAC)
        dispatch_semaphore_wait(impl->sem, DISPATCH_TIME_FOREVER);
#else
        int r;
        do
        {
            r = sem_wait(&impl->sem);
        } while (r != 0 && errno == EINTR);
#endif
	}

	void semaphoreClass::unlock()
	{
		semaphoreImpl *impl = (semaphoreImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		ReleaseSemaphore(impl->sem, 1, nullptr);
#elif defined(CAGE_SYSTEM_MAC)
        dispatch_semaphore_signal(impl->sem);
#else
		sem_post(&impl->sem);
#endif
	}

	holder<semaphoreClass> newSemaphore(uint32 value, uint32 max)
	{
		return detail::systemArena().createImpl<semaphoreClass, semaphoreImpl>(value, max);
	}
}

