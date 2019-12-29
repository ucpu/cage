#ifndef guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_
#define guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_

#include "scopeLock.h"

namespace cage
{
	class CAGE_API Mutex : private Immovable
	{
	public:
		// locking the same mutex again in the same thread is implementation defined behavior
		bool tryLock(); // return true on successfully acquiring the lock
		void lock();
		void unlock();
	};

	CAGE_API Holder<Mutex> newSyncMutex();

	class CAGE_API Barrier : private Immovable
	{
	public:
		void lock();
		void unlock(); // does nothing
	};

	CAGE_API Holder<Barrier> newSyncBarrier(uint32 value);

	class CAGE_API Semaphore : private Immovable
	{
	public:
		void lock(); // decrements value
		void unlock(); // increments value
	};

	CAGE_API Holder<Semaphore> newSyncSemaphore(uint32 value, uint32 max);

	class CAGE_API ConditionalVariableBase : private Immovable
	{
	public:
		void wait(Mutex *mut);
		void wait(Holder<Mutex> &mut) { return wait(mut.get()); }
		void wait(ScopeLock<Mutex> &mut) { return wait(mut.ptr); }
		void signal();
		void broadcast();
	};

	CAGE_API Holder<ConditionalVariableBase> newSyncConditionalBase();

	// this is a compound class containing both mutex and conditional variable
	class CAGE_API ConditionalVariable : private Immovable
	{
	public:
		void lock(); // lock the mutex
		void unlock(); // unlock the mutex and signal (or broadcast) the conditional variable
		void wait(); // unlock the mutex and wait on the conditional variable (beware of spurious wake up)
		void signal(); // signal the conditional variable without touching the mutex
		void broadcast(); // broadcast the conditional variable without touching the mutex
	};

	CAGE_API Holder<ConditionalVariable> newSyncConditional(bool broadcast = false);

	class CAGE_API Thread : private Immovable
	{
	public:
		uint64 id() const;
		bool done() const;
		void wait();
	};

	CAGE_API Holder<Thread> newThread(Delegate<void()> func, const string &threadName);

	CAGE_API void setCurrentThreadName(const string &name);
	CAGE_API string getCurrentThreadName();

	CAGE_API uint32 processorsCount(); // return count of threads, that can physically run simultaneously
	CAGE_API uint64 threadId(); // return id of current thread
	CAGE_API uint64 processId(); // return id of current process
	CAGE_API void threadSleep(uint64 micros);
}

#endif // guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_
