namespace cage
{
	class CAGE_API mutexClass
	{
	public:
		// locking the same mutex again in the same thread is undefined behavior
		void lock();
		void unlock();
	};

	class CAGE_API barrierClass
	{
	public:
		void lock();
		void unlock(); // does nothing
	};

	class CAGE_API semaphoreClass
	{
	public:
		void lock(); // decrements value
		void unlock(); // increments value
	};

	CAGE_API holder<mutexClass> newMutex();
	CAGE_API holder<barrierClass> newBarrier(uint32 value);
	CAGE_API holder<semaphoreClass> newSemaphore(uint32 value, uint32 max);

	template struct CAGE_API scopeLock<mutexClass>;
	template struct CAGE_API scopeLock<barrierClass>;
	template struct CAGE_API scopeLock<semaphoreClass>;
}
