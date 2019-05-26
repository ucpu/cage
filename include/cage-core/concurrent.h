#ifndef guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_
#define guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_

#include "scopeLock.h"

namespace cage
{
	class CAGE_API mutexClass
	{
	public:
		// locking the same mutex again in the same thread is undefined behavior
		bool tryLock(); // return true on successfully acquiring the lock
		void lock();
		void unlock();
	};

	CAGE_API holder<mutexClass> newMutex();

	class CAGE_API barrierClass
	{
	public:
		void lock();
		void unlock(); // does nothing
	};

	CAGE_API holder<barrierClass> newBarrier(uint32 value);

	class CAGE_API semaphoreClass
	{
	public:
		void lock(); // decrements value
		void unlock(); // increments value
	};

	CAGE_API holder<semaphoreClass> newSemaphore(uint32 value, uint32 max);

	class CAGE_API conditionalBaseClass
	{
	public:
		void wait(mutexClass *mut);
		void wait(holder<mutexClass> &mut) { return wait(mut.get()); }
		void wait(scopeLock<mutexClass> &mut) { return wait(mut.ptr); }
		void signal();
		void broadcast();
	};

	CAGE_API holder<conditionalBaseClass> newConditionalBase();

	class CAGE_API conditionalClass
	{
		// this is a compound class containing both mutex and conditional variable
	public:
		void lock(); // lock the mutex
		void unlock(); // unlock the mutex and signal (or broadcast) the conditional variable
		void wait(); // unlock the mutex and wait on the conditional variable (beware of spurious wake up)
		void signal(); // signal the conditional variable without touching the mutex
		void broadcast(); // broadcast the conditional variable without touching the mutex
	};

	CAGE_API holder<conditionalClass> newConditional(bool broadcast = false);

	class CAGE_API threadClass
	{
	public:
		uint64 id() const;
		bool done() const;
		void wait();
	};

	CAGE_API holder<threadClass> newThread(delegate<void()> func, const string &threadName);

	CAGE_API void setCurrentThreadName(const string &name);
	CAGE_API string getCurrentThreadName();

	CAGE_API uint32 processorsCount(); // return count of threads, that can physically run simultaneously
	CAGE_API uint64 threadId(); // return id of current thread
	CAGE_API uint64 processId(); // return id of current process
	CAGE_API void threadSleep(uint64 micros);
}

#endif // guard_concurrent_h_585972b8_5791_4554_b4fa_29342c539280_
