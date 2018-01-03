#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <pthread.h>
#include <cerrno>
#endif

#include "mutex.h"

namespace cage
{
	void mutexClass::lock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		EnterCriticalSection(&impl->cs);
#else
		int r;
		do
		{
			r = pthread_mutex_lock(&impl->mut);
		} while (r != 0 && errno == EINTR);
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
