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
	bool mutexClass::tryLock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		return TryEnterCriticalSection(&impl->cs) != 0;
#else
		int r;
		do
		{
			r = pthread_mutex_trylock(&impl->mut);
			if (r != 0 && errno == EBUSY)
				return false;
		} while (r != 0 && errno == EINTR);
		if (r == 0)
			return true;
		CAGE_THROW_ERROR(codeException, "mutex trylock error", errno);
#endif
	}

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
		if (r != 0)
			CAGE_THROW_ERROR(codeException, "mutex lock error", errno);
#endif
	}

	void mutexClass::unlock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		LeaveCriticalSection(&impl->cs);
#else
		int r = pthread_mutex_unlock(&impl->mut);
		if (r != 0)
			CAGE_THROW_ERROR(codeException, "mutex unlock error", errno);
#endif
	}

	holder<mutexClass> newMutex()
	{
		return detail::systemArena().createImpl<mutexClass, mutexImpl>();
	}
}
