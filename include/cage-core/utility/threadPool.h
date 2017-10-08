#ifndef guard_threadPool_h_85C3A6DCAB82493AB056948639D0AC0A
#define guard_threadPool_h_85C3A6DCAB82493AB056948639D0AC0A

namespace cage
{
	template struct CAGE_API delegate<void(uint32, uint32)>;

	class CAGE_API threadPoolClass
	{
	public:
		delegate<void(uint32, uint32)> function;
		void run();
	};

	CAGE_API holder<threadPoolClass> newThreadPool(const string threadNames = "worker_", uint32 threadsCount = 0);
}

#endif // guard_threadPool_h_85C3A6DCAB82493AB056948639D0AC0A
