#ifndef guard_threadPool_h_85C3A6DCAB82493AB056948639D0AC0A
#define guard_threadPool_h_85C3A6DCAB82493AB056948639D0AC0A

#include "core.h"

namespace cage
{
	class CAGE_CORE_API ThreadPool : private Immovable
	{
	public:
		Delegate<void(uint32 threadIndex, uint32 threadsCount)> function;

		uint32 threadsCount() const;
		void run();
	};

	// threadsCount == 0 -> run in calling thread
	// threadsCount == m -> as many threads as there is processors
	CAGE_CORE_API Holder<ThreadPool> newThreadPool(const string &threadNames = "worker_", uint32 threadsCount = m);

	CAGE_CORE_API std::pair<uint32, uint32> tasksSplit(uint32 groupIndex, uint32 groupsCount, uint32 tasksCount);
}

#endif // guard_threadPool_h_85C3A6DCAB82493AB056948639D0AC0A
