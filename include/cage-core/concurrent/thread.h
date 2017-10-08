namespace cage
{
	class CAGE_API threadClass
	{
	public:
		uint64 id() const;
		bool done() const;
		void wait();
		string getThreadName() const;
	};

	CAGE_API holder<threadClass> newThread(delegate<void()> func, const string &threadName);

	CAGE_API uint32 processorsCount(); // return count of threads, that can physically run simultaneously
	CAGE_API uint64 threadId(); // return id of current thread
	CAGE_API uint64 processId(); // return id of current process
	CAGE_API void threadSleep(uint64 micros);
}

