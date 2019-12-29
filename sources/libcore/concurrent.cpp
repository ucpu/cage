#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

#include <thread>
#include <set>
#include <exception>
#include <cerrno>

#ifdef CAGE_SYSTEM_WINDOWS
#include "incWin.h"
#else
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#endif

#ifdef CAGE_SYSTEM_LINUX
#include <sys/prctl.h>
#endif

#ifdef CAGE_SYSTEM_MAC
#include <dispatch/dispatch.h>
#endif

#include <optick.h>

namespace cage
{
	class mutexImpl : public Mutex
	{
	public:
#ifdef CAGE_SYSTEM_WINDOWS
		SRWLOCK srw;
#else
		pthread_mutex_t mut;
#endif

		mutexImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			InitializeSRWLock(&srw);
#else
			pthread_mutex_init(&mut, nullptr);
#endif
		}

		~mutexImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			// nothing
#else
			pthread_mutex_destroy(&mut);
#endif
		}
	};

	bool Mutex::tryLock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		return TryAcquireSRWLockExclusive(&impl->srw) != 0;
#else
		int r = pthread_mutex_trylock(&impl->mut);
		if (r == EBUSY)
			return false;
		if (r == 0)
			return true;
		CAGE_THROW_ERROR(SystemError, "mutex trylock error", r);
#endif
	}

	void Mutex::lock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		AcquireSRWLockExclusive(&impl->srw);
#else
		int r;
		do
		{
			r = pthread_mutex_lock(&impl->mut);
		} while (r == EINTR);
		if (r != 0)
			CAGE_THROW_ERROR(SystemError, "mutex lock error", r);
#endif
	}

	void Mutex::unlock()
	{
		mutexImpl *impl = (mutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		ReleaseSRWLockExclusive(&impl->srw);
#else
		int r = pthread_mutex_unlock(&impl->mut);
		if (r != 0)
			CAGE_THROW_CRITICAL(SystemError, "mutex unlock error", r);
#endif
	}

	Holder<Mutex> newSyncMutex()
	{
		return detail::systemArena().createImpl<Mutex, mutexImpl>();
	}

	namespace
	{
		class semaphoreImpl : public Semaphore
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

	void Semaphore::lock()
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
		} while (r == EINTR);
#endif
	}

	void Semaphore::unlock()
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

	Holder<Semaphore> newSyncSemaphore(uint32 value, uint32 max)
	{
		return detail::systemArena().createImpl<Semaphore, semaphoreImpl>(value, max);
	}
}

#ifdef CAGE_SYSTEM_MAC

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
		class barrierImpl : public Barrier
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS

			Holder<Mutex> mut;
			Holder<Semaphore> sem1, sem2;
			uint32 total, current;

			barrierImpl(uint32 value) : mut(newSyncMutex()), sem1(newSyncSemaphore(0, value)), sem2(newSyncSemaphore(0, value)), total(value), current(0)
			{}

			~barrierImpl()
			{}

			void lock()
			{
				{
					ScopeLock<Mutex> l(mut);
					if (++current == total)
					{
						for (uint32 i = 0; i < total; i++)
							sem1->unlock();
					}
				}
				sem1->lock();
				{
					ScopeLock<Mutex> l(mut);
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

	void Barrier::lock()
	{
		barrierImpl *impl = (barrierImpl*)this;
		impl->lock();;
	}

	void Barrier::unlock()
	{
		// does nothing
	}

	Holder<Barrier> newSyncBarrier(uint32 value)
	{
		return detail::systemArena().createImpl<Barrier, barrierImpl>(value);
	}

	namespace
	{
		class conditionalBaseImpl : public ConditionalVariableBase
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

		class conditionalImpl : public ConditionalVariable
		{
		public:
			Holder<Mutex> mut;
			Holder<ConditionalVariableBase> cond;
			bool broadcasting;

			conditionalImpl(bool broadcasting) : broadcasting(broadcasting)
			{
				mut = newSyncMutex();
				cond = newSyncConditionalBase();
			}
		};
	}

	void ConditionalVariableBase::wait(Mutex *mut)
	{
		conditionalBaseImpl *impl = (conditionalBaseImpl *)this;
		mutexImpl *m = (mutexImpl*)mut;
#ifdef CAGE_SYSTEM_WINDOWS
		SleepConditionVariableSRW(&impl->cond, &m->srw, INFINITE, 0);
#else
		pthread_cond_wait(&impl->cond, &m->mut);
#endif
	}

	void ConditionalVariableBase::signal()
	{
		conditionalBaseImpl *impl = (conditionalBaseImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeConditionVariable(&impl->cond);
#else
		pthread_cond_signal(&impl->cond);
#endif
	}

	void ConditionalVariableBase::broadcast()
	{
		conditionalBaseImpl *impl = (conditionalBaseImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeAllConditionVariable(&impl->cond);
#else
		pthread_cond_broadcast(&impl->cond);
#endif
	}

	Holder<ConditionalVariableBase> newSyncConditionalBase()
	{
		return detail::systemArena().createImpl<ConditionalVariableBase, conditionalBaseImpl>();
	}

	void ConditionalVariable::lock()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->mut->lock();
	}

	void ConditionalVariable::unlock()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->mut->unlock();
		if (impl->broadcasting)
			impl->cond->broadcast();
		else
			impl->cond->signal();
	}

	void ConditionalVariable::wait()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->cond->wait(impl->mut);
	}

	void ConditionalVariable::signal()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->cond->signal();
	}

	void ConditionalVariable::broadcast()
	{
		conditionalImpl *impl = (conditionalImpl *)this;
		impl->cond->broadcast();
	}

	Holder<ConditionalVariable> newSyncConditional(bool broadcast)
	{
		return detail::systemArena().createImpl<ConditionalVariable, conditionalImpl>(broadcast);
	}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params);
#else
		void *threadFunctionImpl(void *);
#endif

		class threadImpl : public Thread
		{
		public:
			const string threadName;
			const Delegate<void()> function;
			std::exception_ptr exptr;
			uint64 myid;

#ifdef CAGE_SYSTEM_WINDOWS
			HANDLE handle;
#else
			pthread_t handle;
#endif

			threadImpl(Delegate<void()> function, const string &threadName) : threadName(threadName), function(function), myid(m)
			{
#ifdef CAGE_SYSTEM_WINDOWS

				DWORD tid = -1;
				handle = CreateThread(nullptr, 0, &threadFunctionImpl, this, 0, &tid);
				if (!handle)
					CAGE_THROW_ERROR(Exception, "CreateThread");
				myid = uint64(tid);

#else

				if (pthread_create(&handle, nullptr, &threadFunctionImpl, this) != 0)
					CAGE_THROW_ERROR(SystemError, "pthread_create", errno);
				myid = uint64(handle);

#endif
			}

			~threadImpl()
			{
				try
				{
					wait();
				}
				catch (...)
				{
#ifdef CAGE_SYSTEM_WINDOWS
					CloseHandle(handle);
#endif
					CAGE_LOG(SeverityEnum::Critical, "thread", stringizer() + "exception thrown in thread '" + threadName + "' cannot be propagated to the caller thread, terminating now");
					std::terminate();
				}

#ifdef CAGE_SYSTEM_WINDOWS
				CloseHandle(handle);
#endif
			}
		};
	}

	uint64 Thread::id() const
	{
		threadImpl *impl = (threadImpl*)this;
		return impl->myid;
	}

	bool Thread::done() const
	{
		threadImpl *impl = (threadImpl*)this;

#ifdef CAGE_SYSTEM_WINDOWS
		return WaitForSingleObject(impl->handle, 0) == WAIT_OBJECT_0;
#else
		if (impl->handle == (pthread_t)nullptr)
			return true;
#ifdef CAGE_SYSTEM_MAC
		// mac does not have pthread_tryjoin_np
		switch (pthread_kill(impl->handle, 0))
		{
		case 0: return false;
		default: const_cast<Thread*>(this)->wait(); return true;
		}
#else
		switch (int err = pthread_tryjoin_np(impl->handle, nullptr))
		{
		case 0: impl->handle = (pthread_t)nullptr; return true;
		case EBUSY: return false;
		default: CAGE_THROW_ERROR(SystemError, "pthread_tryjoin_np", err);
		}
#endif
#endif
	}

	void Thread::wait()
	{
		threadImpl *impl = (threadImpl*)this;

#ifdef CAGE_SYSTEM_WINDOWS
		WaitForSingleObject(impl->handle, INFINITE);
#else
		if (impl->handle != (pthread_t)nullptr)
		{
			switch (int err = pthread_join(impl->handle, nullptr))
			{
			case 0: impl->handle = (pthread_t)nullptr; break;
			default: CAGE_THROW_ERROR(SystemError, "pthread_join", err);
			}
		}
#endif

		if (impl->exptr)
		{
			std::exception_ptr tmp;
			std::swap(tmp, impl->exptr);
			std::rethrow_exception(tmp);
		}
	}

	Holder<Thread> newThread(Delegate<void()> func, const string &threadName)
	{
		return detail::systemArena().createImpl<Thread, threadImpl>(func, threadName);
	}

	namespace
	{
		string &currentThreadName()
		{
			static thread_local string name;
			return name;
		}
	}

	void setCurrentThreadName(const string &name)
	{
		string oldName = currentThreadName();
		currentThreadName() = name;
		CAGE_LOG(SeverityEnum::Info, "thread", stringizer() + "renamed thread id '" + threadId() + "' to '" + name + "'" + (oldName.empty() ? stringizer() : stringizer() + " was '" + oldName + "'"));

		if (!name.empty())
		{
#ifdef CAGE_SYSTEM_WINDOWS
			static const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
			struct THREADNAME_INFO
			{
				DWORD dwType; // Must be 0x1000.
				LPCSTR szName; // Pointer to name (in user addr space).
				DWORD dwThreadID; // Thread ID (-1=caller thread).
				DWORD dwFlags; // Reserved for future use, must be zero.
			};
#pragma pack(pop)
			THREADNAME_INFO info;
			info.dwType = 0x1000;
			info.szName = name.c_str();
			info.dwThreadID = -1;
			info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
			__try {
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
			}
#pragma warning(pop)
#endif

#ifdef CAGE_SYSTEM_LINUX
			prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#endif
		}
	}

	string getCurrentThreadName()
	{
		string n = currentThreadName();
		if (n.empty())
			return string(threadId());
		else
			return n;
	}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params)
#else
		void *threadFunctionImpl(void *params)
#endif
		{
			threadImpl *impl = (threadImpl*)params;
			setCurrentThreadName(impl->threadName);
			OPTICK_THREAD(impl->threadName.c_str());
			try
			{
				impl->function();
			}
			catch (...)
			{
				impl->exptr = std::current_exception();
				CAGE_LOG(SeverityEnum::Warning, "thread", stringizer() + "unhandled exception in thread '" + getCurrentThreadName() + "'");
			}
			CAGE_LOG(SeverityEnum::Info, "thread", stringizer() + "thread '" + getCurrentThreadName() + "' ended");
#ifdef CAGE_SYSTEM_WINDOWS
			return 0;
#else
			return nullptr;
#endif
		}
	}

	uint32 processorsCount()
	{
		return std::thread::hardware_concurrency();
	}

	uint64 threadId()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		return uint64(GetCurrentThreadId());
#else
		return uint64(pthread_self());
#endif
	}

	uint64 processId()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		return uint64(GetCurrentProcessId());
#else
		return uint64(getpid());
#endif
	}

	void threadSleep(uint64 micros)
	{
#ifdef CAGE_SYSTEM_WINDOWS
		micros /= 1000;
		//if (micros == 0)
		//	micros = 1;
		Sleep((DWORD)micros);
#else
		usleep(micros);
#endif
	}
}
