#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include "../system.h"

namespace cage
{
	namespace
	{
		class semaphoreImpl : public semaphoreClass
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			HANDLE sem;
#else
			sem_t sem;
#endif

			semaphoreImpl(uint32 value, uint32 max)
			{
#ifdef CAGE_SYSTEM_WINDOWS
				sem = CreateSemaphore(nullptr, value, max, nullptr);
#else

				sem_init(&sem, 0, max);
				for (uint32 i = 0; i < value; i++)
					unlock();

#endif
			}

			~semaphoreImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				CloseHandle(sem);
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
#else
		sem_wait(&impl->sem);
#endif
	}

	void semaphoreClass::unlock()
	{
		semaphoreImpl *impl = (semaphoreImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		ReleaseSemaphore(impl->sem, 1, nullptr);
#else
		sem_post(&impl->sem);
#endif
	}

	holder<semaphoreClass> newSemaphore(uint32 value, uint32 max)
	{
		return detail::systemArena().createImpl<semaphoreClass, semaphoreImpl>(value, max);
	}
}