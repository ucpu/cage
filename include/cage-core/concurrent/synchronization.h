namespace cage
{
	class CAGE_API mutexClass
	{
	public:
		// locking the same mutex again in the same thread is undefined behavior
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
}
