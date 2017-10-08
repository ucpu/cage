namespace cage
{
	struct CAGE_API memoryConcurrentPolicyNone
	{
		void lock()
		{}

		void unlock()
		{}
	};

	struct CAGE_API memoryConcurrentPolicyMutex
	{
		memoryConcurrentPolicyMutex();
		void lock();
		void unlock();

	private:
		holdev mutex;
	};
}
