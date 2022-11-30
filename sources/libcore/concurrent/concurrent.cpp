#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
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

#include <thread>
#include <atomic>
#include <exception>
#include <cerrno>

namespace cage
{
	namespace
	{
		class MutexImpl : public Mutex
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			SRWLOCK srw;
#else
			pthread_mutex_t mut;
#endif

			MutexImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				InitializeSRWLock(&srw);
#else
				pthread_mutex_init(&mut, nullptr);
#endif
			}

			~MutexImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				// nothing
#else
				pthread_mutex_destroy(&mut);
#endif
			}
		};
	}

	bool Mutex::tryLock()
	{
		MutexImpl *impl = (MutexImpl *)this;
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
		MutexImpl *impl = (MutexImpl *)this;
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
		MutexImpl *impl = (MutexImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		ReleaseSRWLockExclusive(&impl->srw);
#else
		int r = pthread_mutex_unlock(&impl->mut);
		if (r != 0)
			CAGE_THROW_CRITICAL(SystemError, "mutex unlock error", r);
#endif
	}

	Holder<Mutex> newMutex()
	{
		return systemMemory().createImpl<Mutex, MutexImpl>();
	}

	namespace
	{
		class RwMutexImpl : public RwMutex
		{
		public:
			std::atomic<uint32> v = 0;
			static constexpr uint32 YieldAfter = 50;
			static constexpr uint32 Writer = m;
		};
	}

	void RwMutex::writeLock()
	{
		RwMutexImpl *impl = (RwMutexImpl *)this;
		uint32 attempt = 0;
		while (true)
		{
			uint32 p = impl->v;
			if (p == 0)
			{
				if (impl->v.compare_exchange_weak(p, RwMutexImpl::Writer))
					return;
			}
			if (++attempt >= RwMutexImpl::YieldAfter)
			{
				threadYield();
				attempt = 0;
			}
		}
	}

	void RwMutex::readLock()
	{
		RwMutexImpl *impl = (RwMutexImpl *)this;
		uint32 attempt = 0;
		while (true)
		{
			uint32 p = impl->v;
			if (p != RwMutexImpl::Writer)
			{
				if (impl->v.compare_exchange_weak(p, p + 1))
					return;
			}
			if (++attempt >= RwMutexImpl::YieldAfter)
			{
				threadYield();
				attempt = 0;
			}
		}
	}

	void RwMutex::unlock()
	{
		RwMutexImpl *impl = (RwMutexImpl *)this;
		uint32 p = impl->v;
		if (p == RwMutexImpl::Writer)
			impl->v = 0;
		else
			impl->v--;
	}

	Holder<RwMutex> newRwMutex()
	{
		return systemMemory().createImpl<RwMutex, RwMutexImpl>();
	}

	namespace
	{
		class SemaphoreImpl : public Semaphore
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			HANDLE sem;
#elif defined(CAGE_SYSTEM_MAC)
			dispatch_semaphore_t sem;
#else
			sem_t sem;
#endif

			SemaphoreImpl(uint32 value, uint32 max)
			{
#ifdef CAGE_SYSTEM_WINDOWS
				sem = CreateSemaphore(nullptr, value, max, nullptr);
#elif defined(CAGE_SYSTEM_MAC)
				sem = dispatch_semaphore_create(value);
#else
				sem_init(&sem, 0, value);
#endif
			}

			~SemaphoreImpl()
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
		SemaphoreImpl *impl = (SemaphoreImpl *)this;
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
		SemaphoreImpl *impl = (SemaphoreImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		ReleaseSemaphore(impl->sem, 1, nullptr);
#elif defined(CAGE_SYSTEM_MAC)
		dispatch_semaphore_signal(impl->sem);
#else
		sem_post(&impl->sem);
#endif
	}

	Holder<Semaphore> newSemaphore(uint32 value, uint32 max)
	{
		return systemMemory().createImpl<Semaphore, SemaphoreImpl>(value, max);
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
		class BarrierImpl : public Barrier
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS

#if 0
			// old implementation compatible with windows before version 8

			Holder<Mutex> mut;
			Holder<Semaphore> sem1, sem2;
			uint32 total, current;

			explicit BarrierImpl(uint32 value) : mut(newMutex()), sem1(newSemaphore(0, value)), sem2(newSemaphore(0, value)), total(value), current(0)
			{}

			~BarrierImpl()
			{}

			void lock()
			{
				{
					ScopeLock l(mut);
					if (++current == total)
					{
						for (uint32 i = 0; i < total; i++)
							sem1->unlock();
					}
				}
				sem1->lock();
				{
					ScopeLock l(mut);
					if (--current == 0)
					{
						for (uint32 i = 0; i < total; i++)
							sem2->unlock();
					}
				}
				sem2->lock();
			}

#else
			// new implementation that requires windows 8 or later

			SYNCHRONIZATION_BARRIER bar;

			explicit BarrierImpl(uint32 value)
			{
				if (InitializeSynchronizationBarrier(&bar, value, 0) != TRUE)
					CAGE_THROW_ERROR(SystemError, "InitializeSynchronizationBarrier", GetLastError());
			}

			~BarrierImpl()
			{
				DeleteSynchronizationBarrier(&bar);
			}

			void lock()
			{
				EnterSynchronizationBarrier(&bar, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
			}

#endif // windows version compatibility

#else

			pthread_barrier_t bar;

			BarrierImpl(uint32 value)
			{
				pthread_barrier_init(&bar, NULL, value);
			}

			~BarrierImpl()
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
		BarrierImpl *impl = (BarrierImpl*)this;
		impl->lock();
	}

	Holder<Barrier> newBarrier(uint32 value)
	{
		return systemMemory().createImpl<Barrier, BarrierImpl>(value);
	}

	namespace
	{
		class ConditionalVariableBaseImpl : public ConditionalVariableBase
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			CONDITION_VARIABLE cond;
#else
			pthread_cond_t cond;
#endif

			ConditionalVariableBaseImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				InitializeConditionVariable(&cond);
#else
				pthread_cond_init(&cond, nullptr);
#endif
			}

			~ConditionalVariableBaseImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				// do nothing
#else
				pthread_cond_destroy(&cond);
#endif
			}
		};

		class ConditionalVariableImpl : public ConditionalVariable
		{
		public:
			Holder<Mutex> mut;
			Holder<ConditionalVariableBase> cond;
			bool broadcasting;

			ConditionalVariableImpl(bool broadcasting) : broadcasting(broadcasting)
			{
				mut = newMutex();
				cond = newConditionalVariableBase();
			}
		};
	}

	void ConditionalVariableBase::wait(Mutex *mut)
	{
		ConditionalVariableBaseImpl *impl = (ConditionalVariableBaseImpl *)this;
		MutexImpl *m = (MutexImpl*)mut;
#ifdef CAGE_SYSTEM_WINDOWS
		SleepConditionVariableSRW(&impl->cond, &m->srw, INFINITE, 0);
#else
		pthread_cond_wait(&impl->cond, &m->mut);
#endif
	}

	void ConditionalVariableBase::signal()
	{
		ConditionalVariableBaseImpl *impl = (ConditionalVariableBaseImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeConditionVariable(&impl->cond);
#else
		pthread_cond_signal(&impl->cond);
#endif
	}

	void ConditionalVariableBase::broadcast()
	{
		ConditionalVariableBaseImpl *impl = (ConditionalVariableBaseImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeAllConditionVariable(&impl->cond);
#else
		pthread_cond_broadcast(&impl->cond);
#endif
	}

	Holder<ConditionalVariableBase> newConditionalVariableBase()
	{
		return systemMemory().createImpl<ConditionalVariableBase, ConditionalVariableBaseImpl>();
	}

	void ConditionalVariable::lock()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
		impl->mut->lock();
	}

	void ConditionalVariable::unlock()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
		impl->mut->unlock();
		if (impl->broadcasting)
			impl->cond->broadcast();
		else
			impl->cond->signal();
	}

	void ConditionalVariable::wait()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
		impl->cond->wait(impl->mut);
	}

	void ConditionalVariable::signal()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
		impl->cond->signal();
	}

	void ConditionalVariable::broadcast()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
		impl->cond->broadcast();
	}

	Holder<ConditionalVariable> newConditionalVariable(bool broadcast)
	{
		return systemMemory().createImpl<ConditionalVariable, ConditionalVariableImpl>(broadcast);
	}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params);
#else
		void *threadFunctionImpl(void *);
#endif

		class ThreadImpl : public Thread
#ifdef CAGE_SYSTEM_WINDOWS
			, public AutoHandle
#endif
		{
		public:
			const String threadName;
			const Delegate<void()> function;
			std::exception_ptr exptr = nullptr;
			uint64 myid = m;

#ifndef CAGE_SYSTEM_WINDOWS
			pthread_t handle;
#endif

			ThreadImpl(Delegate<void()> function, const String &threadName) : threadName(threadName), function(function)
			{
#ifdef CAGE_SYSTEM_WINDOWS

				DWORD tid = -1;
				// reserve 8 MB of memory for its stack to match linux default
				handle = CreateThread(nullptr, 8 * 1024 * 1024, &threadFunctionImpl, this, 0, &tid);
				if (!handle)
				{
					auto err = GetLastError();
					CAGE_THROW_ERROR(SystemError, "CreateThread", err);
				}
				myid = uint64(tid);

#else

				if (pthread_create(&handle, nullptr, &threadFunctionImpl, this) != 0)
					CAGE_THROW_ERROR(SystemError, "pthread_create", errno);
				myid = uint64(handle);

#endif
			}

			~ThreadImpl()
			{
				try
				{
					wait();
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Critical, "thread", Stringizer() + "exception thrown in thread '" + threadName + "' was not propagated to the caller thread (missing call to wait), terminating now");
					detail::logCurrentCaughtException();
					detail::terminate();
				}
			}
		};
	}

	uint64 Thread::id() const
	{
		const ThreadImpl *impl = (const ThreadImpl *)this;
		return impl->myid;
	}

	bool Thread::done() const
	{
		const ThreadImpl *impl = (const ThreadImpl *)this;

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
		default: const_cast<Thread *>(this)->wait(); return true;
		}
#else
		switch (int err = pthread_tryjoin_np(impl->handle, nullptr))
		{
		case 0: const_cast<ThreadImpl *>(impl)->handle = (pthread_t)nullptr; return true;
		case EBUSY: return false;
		default: CAGE_THROW_ERROR(SystemError, "pthread_tryjoin_np", err);
		}
#endif
#endif
	}

	void Thread::wait()
	{
		ThreadImpl *impl = (ThreadImpl *)this;

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
			std::exception_ptr tmp = nullptr;
			std::swap(tmp, impl->exptr);
			std::rethrow_exception(tmp);
		}
	}

	Holder<Thread> newThread(Delegate<void()> func, const String &threadName)
	{
		return systemMemory().createImpl<Thread, ThreadImpl>(func, threadName);
	}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params)
#else
		void *threadFunctionImpl(void *params)
#endif
		{
			ThreadImpl *impl = (ThreadImpl *)params;
			currentThreadName(impl->threadName);
			try
			{
				impl->function();
			}
			catch (...)
			{
				impl->exptr = std::current_exception();
				CAGE_LOG(SeverityEnum::Warning, "thread", Stringizer() + "unhandled exception in thread '" + currentThreadName() + "'");
				detail::logCurrentCaughtException();
			}
			CAGE_LOG_DEBUG(SeverityEnum::Info, "thread", Stringizer() + "thread '" + currentThreadName() + "' ended");
#ifdef CAGE_SYSTEM_WINDOWS
			return 0;
#else
			return nullptr;
#endif
		}

		String &currentThreadName_()
		{
			thread_local String name;
			return name;
		}
	}

#ifdef CAGE_PROFILING_ENABLED
	void profilingThreadName() noexcept;
#endif

	void currentThreadName(const String &name)
	{
		const String oldName = currentThreadName_();
		currentThreadName_() = name;

		if (!name.empty())
		{
#ifdef CAGE_SYSTEM_WINDOWS
			static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
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

#ifdef CAGE_PROFILING_ENABLED
		profilingThreadName();
#endif

		CAGE_LOG_DEBUG(SeverityEnum::Info, "thread", Stringizer() + "renamed thread id '" + currentThreadId() + "' to '" + name + "'" + (oldName.empty() ? Stringizer() : Stringizer() + " was '" + oldName + "'"));
	}

	String currentThreadName()
	{
		String n = currentThreadName_();
		if (n.empty())
			return Stringizer() + currentThreadId();
		else
			return n;
	}

	uint64 currentThreadId()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		return uint64(GetCurrentThreadId());
#else
		return uint64(pthread_self());
#endif
	}

	uint64 currentProcessId()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		return uint64(GetCurrentProcessId());
#else
		return uint64(getpid());
#endif
	}

	uint32 processorsCount()
	{
		return std::thread::hardware_concurrency();
	}

	void threadSleep(uint64 micros)
	{
#ifdef CAGE_SYSTEM_WINDOWS
		// windows Sleep has millisecond precision only
		if (micros < 1000)
		{
			// we yield the thread and potentially busy wait for the remaining duration
			static_assert(sizeof(uint64) == sizeof(LARGE_INTEGER));
			uint64 freq = 0, begin = 0, end = 0;
			QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
			QueryPerformanceCounter((LARGE_INTEGER *)&begin);
			while (true)
			{
				Sleep(0); // yields the thread
				QueryPerformanceCounter((LARGE_INTEGER *)&end);
				const uint64 elapsed = 1000000 * (end - begin) / freq;
				if (elapsed >= micros)
					return;
			}
		}
		else
			Sleep(numeric_cast<DWORD>(micros / 1000));
#else
		usleep(micros);
#endif
	}

	void threadYield()
	{
		std::this_thread::yield();
	}
}
