#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include "../system.h"

namespace cage
{
	namespace
	{
		class mutexImpl : public mutexClass
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			CRITICAL_SECTION cs;
#else
			pthread_mutex_t mut;
#endif

			mutexImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				InitializeCriticalSection(&cs);
#else
				pthread_mutex_init(&mut, nullptr);
#endif
			}

			~mutexImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				DeleteCriticalSection(&cs);
#else
				pthread_mutex_destroy(&mut);
#endif
			}
		};
	}

	void mutexClass::lock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		EnterCriticalSection(&impl->cs);
#else
		pthread_mutex_lock(&impl->mut);
#endif
	}

	void mutexClass::unlock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		LeaveCriticalSection(&impl->cs);
#else
		pthread_mutex_unlock(&impl->mut);
#endif
	}

	holder<mutexClass> newMutex()
	{
		return detail::systemArena().createImpl<mutexClass, mutexImpl>();
	}
}