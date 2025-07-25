#include <atomic>
#include <cerrno>
#include <exception>
#include <mutex>
#include <thread>

#ifdef CAGE_SYSTEM_WINDOWS
	//#ifndef WIN32_LEAN_AND_MEAN
	//	#define WIN32_LEAN_AND_MEAN
	//#endif
	#ifndef VC_EXTRALEAN
		#define VC_EXTRALEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include "../windowsAutoHandle.h"
#else
	#include <pthread.h>
	#include <semaphore.h>
	#include <sys/types.h>
	#include <unistd.h>
#endif

#ifdef CAGE_SYSTEM_LINUX
	#include <sys/prctl.h>
#endif

#ifdef CAGE_SYSTEM_MAC
	#include <dispatch/dispatch.h>
#endif

#include <cage-core/concurrent.h>
#include <cage-core/debug.h>
#include <cage-core/timer.h>

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
			static constexpr uint32 YieldAfter = 100;
			static constexpr uint32 Writer = m;
		};
	}

	void RwMutex::writeLock()
	{
		RwMutexImpl *impl = (RwMutexImpl *)this;
		uint32 attempt = 0;
		while (true)
		{
			uint32 p = 0;
			if (impl->v.compare_exchange_weak(p, RwMutexImpl::Writer, std::memory_order_acquire))
				return;
			if (++attempt >= RwMutexImpl::YieldAfter)
				threadYield();
		}
	}

	void RwMutex::readLock()
	{
		RwMutexImpl *impl = (RwMutexImpl *)this;
		uint32 attempt = 0;
		while (true)
		{
			uint32 p = impl->v.load(std::memory_order_relaxed);
			if (p != RwMutexImpl::Writer && impl->v.compare_exchange_weak(p, p + 1, std::memory_order_acquire))
				return;
			if (++attempt >= RwMutexImpl::YieldAfter)
				threadYield();
		}
	}

	void RwMutex::unlock()
	{
		RwMutexImpl *impl = (RwMutexImpl *)this;
		if (impl->v.load(std::memory_order_relaxed) == RwMutexImpl::Writer)
			impl->v.store(0, std::memory_order_release);
		else
			impl->v.fetch_sub(1, std::memory_order_release);
	}

	Holder<RwMutex> newRwMutex()
	{
		return systemMemory().createImpl<RwMutex, RwMutexImpl>();
	}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS

		// this implementation does not depend on crt
		class RecursiveMutexImplWindows : private Immovable
		{
		public:
			RecursiveMutexImplWindows() { InitializeSRWLock(&srw); }

			bool try_lock()
			{
				const DWORD currentId = GetCurrentThreadId();
				if (currentId == ownerThreadId)
				{
					recursionCount++;
					return true;
				}
				if (TryAcquireSRWLockExclusive(&srw))
				{
					ownerThreadId = currentId;
					recursionCount = 1;
					return true;
				}
				return false;
			}

			void lock()
			{
				const DWORD currentId = GetCurrentThreadId();
				if (currentId == ownerThreadId)
				{
					recursionCount++;
					return;
				}
				AcquireSRWLockExclusive(&srw);
				ownerThreadId = currentId;
				recursionCount = 1;
			}

			void unlock()
			{
				const DWORD currentId = GetCurrentThreadId();
				if (currentId != ownerThreadId)
				{
					CAGE_THROW_CRITICAL(Exception, "unlocking recursive mutex from different thread");
				}
				if (--recursionCount == 0)
				{
					ownerThreadId = 0;
					ReleaseSRWLockExclusive(&srw);
				}
			}

		private:
			SRWLOCK srw;
			DWORD ownerThreadId = 0;
			sint32 recursionCount = 0;
		};

#endif // CAGE_SYSTEM_WINDOWS

		class RecursiveMutexImpl : public RecursiveMutex
		{
		public:
			// the std::recursive_mutex gets internally corrupted on some computers
			// it happened to me after installing intel graphics profiler (likely causes a dll injection, or some similar stuff)
#ifdef CAGE_SYSTEM_WINDOWS
			RecursiveMutexImplWindows mut;
#else
			std::recursive_mutex mut;
#endif // CAGE_SYSTEM_WINDOWS
		};
	}

	bool RecursiveMutex::tryLock()
	{
		RecursiveMutexImpl *impl = (RecursiveMutexImpl *)this;
		return impl->mut.try_lock();
	}

	void RecursiveMutex::lock()
	{
		RecursiveMutexImpl *impl = (RecursiveMutexImpl *)this;
		impl->mut.lock();
	}

	void RecursiveMutex::unlock()
	{
		RecursiveMutexImpl *impl = (RecursiveMutexImpl *)this;
		impl->mut.unlock();
	}

	Holder<RecursiveMutex> newRecursiveMutex()
	{
		return systemMemory().createImpl<RecursiveMutex, RecursiveMutexImpl>();
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
			SYNCHRONIZATION_BARRIER bar;

			explicit BarrierImpl(uint32 value)
			{
				if (InitializeSynchronizationBarrier(&bar, value, 0) != TRUE)
					CAGE_THROW_ERROR(SystemError, "InitializeSynchronizationBarrier", GetLastError());
			}

			~BarrierImpl() { DeleteSynchronizationBarrier(&bar); }

			void lock() { EnterSynchronizationBarrier(&bar, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY); }
#else
			pthread_barrier_t bar;

			BarrierImpl(uint32 value) { pthread_barrier_init(&bar, NULL, value); }

			~BarrierImpl() { pthread_barrier_destroy(&bar); }

			void lock() { pthread_barrier_wait(&bar); }
#endif
		};
	}

	void Barrier::lock()
	{
		BarrierImpl *impl = (BarrierImpl *)this;
		impl->lock();
	}

	Holder<Barrier> newBarrier(uint32 value)
	{
		return systemMemory().createImpl<Barrier, BarrierImpl>(value);
	}

	namespace
	{
		class ConditionalVariableImpl : public ConditionalVariable
		{
		public:
#ifdef CAGE_SYSTEM_WINDOWS
			CONDITION_VARIABLE cond;
#else
			pthread_cond_t cond;
#endif

			ConditionalVariableImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				InitializeConditionVariable(&cond);
#else
				pthread_cond_init(&cond, nullptr);
#endif
			}

			~ConditionalVariableImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				// do nothing
#else
				pthread_cond_destroy(&cond);
#endif
			}
		};
	}

	void ConditionalVariable::wait(Mutex *mut)
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
		MutexImpl *m = (MutexImpl *)mut;
#ifdef CAGE_SYSTEM_WINDOWS
		SleepConditionVariableSRW(&impl->cond, &m->srw, INFINITE, 0);
#else
		pthread_cond_wait(&impl->cond, &m->mut);
#endif
	}

	void ConditionalVariable::signal()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeConditionVariable(&impl->cond);
#else
		pthread_cond_signal(&impl->cond);
#endif
	}

	void ConditionalVariable::broadcast()
	{
		ConditionalVariableImpl *impl = (ConditionalVariableImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		WakeAllConditionVariable(&impl->cond);
#else
		pthread_cond_broadcast(&impl->cond);
#endif
	}

	Holder<ConditionalVariable> newConditionalVariable()
	{
		return systemMemory().createImpl<ConditionalVariable, ConditionalVariableImpl>();
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
			,
						   public AutoHandle
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
				// reserve 8 MB of memory for its stack to match linux default
				static constexpr uintPtr StackSize = 8 * 1024 * 1024;

#ifdef CAGE_SYSTEM_WINDOWS

				DWORD tid = -1;
				handle = CreateThread(nullptr, StackSize, &threadFunctionImpl, this, 0, &tid);
				if (!handle)
				{
					auto err = GetLastError();
					CAGE_THROW_ERROR(SystemError, "CreateThread", err);
				}
				myid = uint64(tid);

#else

				pthread_attr_t attr = {};
				if (pthread_attr_init(&attr) != 0)
					CAGE_THROW_ERROR(SystemError, "pthread_attr_init", errno);
				if (pthread_attr_setstacksize(&attr, StackSize) != 0)
					CAGE_THROW_ERROR(SystemError, "pthread_attr_setstacksize", errno);
				if (pthread_create(&handle, &attr, &threadFunctionImpl, this) != 0)
					CAGE_THROW_ERROR(SystemError, "pthread_create", errno);
				myid = uint64(handle);
				pthread_attr_destroy(&attr);

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
					detail::logCurrentCaughtException();
					CAGE_LOG(SeverityEnum::Critical, "thread", Stringizer() + "exception thrown in thread " + threadName + " was not propagated to the caller thread (missing call to wait)");
					detail::irrecoverableError("exception in ~ThreadImpl");
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
		if (impl->handle == (pthread_t) nullptr)
			return true;
	#ifdef CAGE_SYSTEM_MAC
		// mac does not have pthread_tryjoin_np
		switch (pthread_kill(impl->handle, 0))
		{
			case 0:
				return false;
			default:
				const_cast<Thread *>(this)->wait();
				return true;
		}
	#else
		switch (int err = pthread_tryjoin_np(impl->handle, nullptr))
		{
			case 0:
				const_cast<ThreadImpl *>(impl)->handle = (pthread_t) nullptr;
				return true;
			case EBUSY:
				return false;
			default:
				CAGE_THROW_ERROR(SystemError, "pthread_tryjoin_np", err);
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
		if (impl->handle != (pthread_t) nullptr)
		{
			switch (int err = pthread_join(impl->handle, nullptr))
			{
				case 0:
					impl->handle = (pthread_t) nullptr;
					break;
				default:
					CAGE_THROW_ERROR(SystemError, "pthread_join", err);
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
		void currentThreadStackGuardLimit()
		{
			ULONG v = 128 * 1024;
			SetThreadStackGuarantee(&v);
		}

		struct SetupStackGuardLimit
		{
			SetupStackGuardLimit() { currentThreadStackGuardLimit(); }
		} setupStackGuardLimit;
#endif

#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params)
#else
		void *threadFunctionImpl(void *params)
#endif
		{
			ThreadImpl *impl = (ThreadImpl *)params;
#ifdef CAGE_SYSTEM_WINDOWS
			currentThreadStackGuardLimit();
#endif
			currentThreadName(impl->threadName);
			try
			{
				impl->function();
			}
			catch (...)
			{
				impl->exptr = std::current_exception();
				CAGE_LOG(SeverityEnum::Warning, "thread", Stringizer() + "unhandled exception in thread " + currentThreadName());
				detail::logCurrentCaughtException();
			}
			CAGE_LOG_DEBUG(SeverityEnum::Info, "thread", Stringizer() + "thread " + currentThreadName() + " ended");
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
	void profilingThreadName();
#endif

	void currentThreadName(const String &name)
	{
#ifdef CAGE_DEBUG
		const String oldName = currentThreadName_();
#endif

		currentThreadName_() = name;

		if (!name.empty())
		{
#ifdef CAGE_SYSTEM_WINDOWS
			static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
	#pragma pack(push, 8)
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
	#pragma warning(disable : 6320 6322)
			__try
			{
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{}
	#pragma warning(pop)
#endif

#ifdef CAGE_SYSTEM_LINUX
			prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#endif
		}

#ifdef CAGE_PROFILING_ENABLED
		profilingThreadName();
#endif

		CAGE_LOG_DEBUG(SeverityEnum::Info, "thread", Stringizer() + "renamed thread id: " + currentThreadId() + (oldName.empty() ? Stringizer() : Stringizer() + ", from: " + oldName) + ", to: " + name);
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

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS

		// the perfect sleep function:
		// https://blog.bearcats.nl/perfect-sleep-function/

		// temporarily changes system scheduler precision
		struct AutoTimeBeginPeriod : private Immovable
		{
			UINT param = 0;

			AutoTimeBeginPeriod(UINT p) : param(p)
			{
				CAGE_ASSERT(param != 0);
				auto err = timeBeginPeriod(p);
				if (err != TIMERR_NOERROR)
					param = 0; // disable call to timeEndPeriod
			}

			~AutoTimeBeginPeriod()
			{
				if (param != 0)
					timeEndPeriod(param);
			}
		};

		// provides high precision sleep
		struct WinTimer : private AutoHandle
		{
			Holder<Timer> tmr = newTimer();
			TIMECAPS caps = {};

			WinTimer()
			{
				handle = CreateWaitableTimerEx(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
				if (!handle)
				{
					auto err = GetLastError();
					CAGE_LOG(SeverityEnum::Warning, "threadSleep", Stringizer() + "CreateWaitableTimerEx failed with code: " + (sint64)err + ", thread sleeps will be less accurate");
				}

				{
					const MMRESULT err = timeGetDevCaps(&caps, sizeof(caps));
					if (err != MMSYSERR_NOERROR)
						CAGE_THROW_ERROR(SystemError, "timeGetDevCaps", err);
				}
			}

			void sleep(uint64 micros)
			{
				const uint64 start = tmr->duration();
				const uint64 target = start + micros;

				// increase scheduler precision
				AutoTimeBeginPeriod timePeriod(caps.wPeriodMin);

				// efficient sleep up until one scheduler period before the target time
				const uint64 threshold = caps.wPeriodMin * uint64(1'200);
				if (micros > threshold)
				{
					if (handle)
					{
						LARGE_INTEGER due;
						due.QuadPart = -(sint64)((micros - threshold) * 10); // micros -> 100s nanoseconds, negative to make the sleep relative
						if (!SetWaitableTimerEx(handle, &due, 0, nullptr, nullptr, nullptr, 0))
						{
							auto err = GetLastError();
							CAGE_THROW_ERROR(SystemError, "SetWaitableTimerEx", err);
						}
						WaitForSingleObject(handle, INFINITE);
					}
					else
					{ // fallback in case that the high-precision timer is not available
						const uint64 t = (micros - threshold) / 1000;
						if (t > 0)
							Sleep(t);
					}
				}

				// spin the rest of the time
				while (tmr->duration() < target)
					threadYield();
			}
		};
#endif
	}

	void threadSleep(uint64 micros)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		// windows Sleep has millisecond precision only
		thread_local WinTimer timer;
		timer.sleep(micros);

#else

		usleep(micros);

#endif
	}

	void threadYield()
	{
		std::this_thread::yield();
	}
}
