#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <pthread.h>
#endif

#include "mutex.h"

namespace cage
{
	namespace
	{
		class conditionalBaseImpl : public conditionalBaseClass
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			CONDITION_VARIABLE cond;
#else
			pthread_cond_t cond;
#endif

			conditionalBaseImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				InitializeConditionVariable(&cond);
#else
				pthread_cond_init(&cond, nullptr);
#endif
			}

			~conditionalBaseImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				// do nothing
#else
				pthread_cond_destroy(&cond);
#endif
			}
		};

		class conditionalImpl : public conditionalClass
		{
		public:
			holder<mutexClass> mut;
			holder<conditionalBaseClass> cond;
			bool broadcasting;

			conditionalImpl(bool broadcasting) : broadcasting(broadcasting)
			{
				mut = newMutex();
				cond = newConditionalBase();
			}
		};
	}

	void conditionalBaseClass::wait(mutexClass *mut)
	{
		conditionalBaseImpl *impl = (conditionalBaseImpl *)this;
		mutexImpl *m = (mutexImpl*)mut;
#ifdef CAGE_SYSTEM_WINDOWS
		SleepConditionVariableCS(&impl->cond, &m->cs, INFINITE);
#else
		pthread_cond_wait(&impl->cond, &m->mut);
#endif
	}

	void conditionalBaseClass::signal()
	{
		conditionalBaseImpl *impl = (conditionalBaseImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeConditionVariable(&impl->cond);
#else
		pthread_cond_signal(&impl->cond);
#endif
	}

	void conditionalBaseClass::broadcast()
	{
		conditionalBaseImpl *impl = (conditionalBaseImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeAllConditionVariable(&impl->cond);
#else
		pthread_cond_broadcast(&impl->cond);
#endif
	}

	holder<conditionalBaseClass> newConditionalBase()
	{
		return detail::systemArena().createImpl<conditionalBaseClass, conditionalBaseImpl>();
	}

	void conditionalClass::lock()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->mut->lock();
	}

	void conditionalClass::unlock()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		if (impl->broadcasting)
			impl->cond->broadcast();
		else
			impl->cond->signal();
		impl->mut->unlock();
	}

	void conditionalClass::wait()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->cond->wait(impl->mut);
	}

	void conditionalClass::signal()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->cond->signal();
	}

	void conditionalClass::broadcast()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->cond->broadcast();
	}

	holder<conditionalClass> newConditional(bool broadcast)
	{
		return detail::systemArena().createImpl<conditionalClass, conditionalImpl>(broadcast);
	}
}
