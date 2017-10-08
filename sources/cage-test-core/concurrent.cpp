#include "main.h"
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadPool.h>

namespace
{
	uint32 counterGlobal;
	CAGE_THREAD_LOCAL_STORAGE uint32 counterLocal;
	mutexClass *mutexGlobal;
	barrierClass *barrierGlobal;

	void mutexTest(uint32 idx, uint32)
	{
		scopeLock<mutexClass> l(mutexGlobal);
		counterGlobal += idx;
	}

	void barrierTest(uint32 idx, uint32)
	{
		counterLocal += idx;
	}

	void barrierTestFinal(uint32 idx, uint32)
	{
		CAGE_TEST(counterLocal == 100 * idx);
		{
			scopeLock<mutexClass> l(mutexGlobal);
			counterGlobal += idx;
		}
	}

	void threadTest()
	{
		for (uint32 i = 0; i < 100; i++)
			scopeLock<barrierClass> l(barrierGlobal);
	}
}

void testConcurrent()
{
	CAGE_TESTCASE("concurrent");
	holder<barrierClass> barrier = newBarrier(4);
	holder<mutexClass> mutex = newMutex();
	barrierGlobal = barrier.get();
	mutexGlobal = mutex.get();

	{
		CAGE_TESTCASE("barrier");
		holder<threadClass> thrs[4];
		for (uint32 i = 0; i < 4; i++)
			thrs[i] = newThread(delegate<void()>().bind<&threadTest>(), string() + "worker_" + i);
		for (uint32 i = 0; i < 4; i++)
			thrs[i]->wait();
	}

	{
		CAGE_TESTCASE("thread-pool");
		{
			CAGE_TESTCASE("global variable");
			counterGlobal = 0;
			holder<threadPoolClass> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&mutexTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			CAGE_TEST(counterGlobal == 600);
		}
		{
			CAGE_TESTCASE("thread local variable");
			counterGlobal = 0;
			holder<threadPoolClass> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&barrierTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			thrs->function.bind<&barrierTestFinal>();
			thrs->run();
			CAGE_TEST(counterLocal == 0);
			CAGE_TEST(counterGlobal == 6);
		}
	}
}