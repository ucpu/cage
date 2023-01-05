#ifndef guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_
#define guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_

#include "scopeLock.h"

namespace cage
{
	// all the classes here are for inter-thread synchronization only
	// do not use for inter-process synchronization

	// fully exclusive lock
	// locking the same Mutex again in the same thread is forbidden
	class CAGE_CORE_API Mutex : private Immovable
	{
	public:
		bool tryLock(); // return true on successfully acquiring the lock
		void lock();
		void unlock();
	};

	CAGE_CORE_API Holder<Mutex> newMutex();

	// exclusive writer or multiple readers lock
	// starts as spinlock and yields after several failed attempts
	// locking the same RwMutex again in the same thread is forbidden
	class CAGE_CORE_API RwMutex : private Immovable
	{
	public:
		void writeLock();
		void readLock();
		void unlock();
	};

	CAGE_CORE_API Holder<RwMutex> newRwMutex();

	// fully exclusive lock
	// RecursiveMutex may be locked again in the same thread
	class CAGE_CORE_API RecursiveMutex : private Immovable
	{
	public:
		bool tryLock(); // return true on successfully acquiring the lock
		void lock();
		void unlock();
	};

	CAGE_CORE_API Holder<RecursiveMutex> newRecursiveMutex();

	// blocks all threads until the threshold is reached
	class CAGE_CORE_API Barrier : private Immovable
	{
	public:
		void lock();
		void unlock() {}; // does nothing
	};

	CAGE_CORE_API Holder<Barrier> newBarrier(uint32 threshold);

	// exclusive lock with counter
	// values above zero allows that many threads to enter
	// values below zero blocks further threads
	class CAGE_CORE_API Semaphore : private Immovable
	{
	public:
		void lock(); // decrements value
		void unlock(); // increments value
	};

	CAGE_CORE_API Holder<Semaphore> newSemaphore(uint32 value, uint32 max);

	// unlock a mutex and atomically reacquire it when signaled
	class CAGE_CORE_API ConditionalVariable : private Immovable
	{
	public:
		void wait(Mutex *mut);
		void wait(Holder<Mutex> &mut) { return wait(+mut); }
		void wait(ScopeLock<Mutex> &mut) { return wait(mut.ptr); }
		void signal();
		void broadcast();
	};

	CAGE_CORE_API Holder<ConditionalVariable> newConditionalVariable();

	class CAGE_CORE_API Thread : private Immovable
	{
	public:
		uint64 id() const;
		bool done() const;
		void wait();
	};

	CAGE_CORE_API Holder<Thread> newThread(Delegate<void()> func, const String &threadName);

	CAGE_CORE_API void currentThreadName(const String &name);
	CAGE_CORE_API String currentThreadName();
	CAGE_CORE_API uint64 currentThreadId();
	CAGE_CORE_API uint64 currentProcessId();
	CAGE_CORE_API uint32 processorsCount(); // return number of threads that can physically run simultaneously
	CAGE_CORE_API void threadSleep(uint64 micros);
	CAGE_CORE_API void threadYield();
}

#endif // guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_
