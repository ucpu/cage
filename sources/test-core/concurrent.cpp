#include "main.h"
#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>

namespace
{
	uint32 counterGlobal;
	thread_local uint32 counterLocal;
	syncMutex *mutexGlobal;
	syncSemaphore *semaphoreGlobal;
	syncBarrier *barrierGlobal;

	void threadTest()
	{
		for (uint32 i = 0; i < 100; i++)
			scopeLock<syncBarrier> l(barrierGlobal);
	}

	void mutexTest(uint32 idx, uint32)
	{
		scopeLock<syncMutex> l(mutexGlobal);
		counterGlobal += idx;
	}

	void semaphoreTest(uint32 idx, uint32)
	{
		scopeLock<syncSemaphore> l(semaphoreGlobal);
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
			scopeLock<syncMutex> l(mutexGlobal);
			counterGlobal += idx;
		}
	}

	void longTimeTest(uint32 idx, uint32)
	{
		for (uint32 i = 0; i < 10; i++)
		{
			threadSleep(2000);
			{
				scopeLock<syncMutex> l(mutexGlobal);
				counterGlobal += 1;
			}
		}
	}

	void tryLockTest()
	{
		if (scopeLock<syncMutex>(mutexGlobal, true))
		{
			CAGE_TEST(false);
		}
	}
}

void testConcurrent()
{
	CAGE_TESTCASE("concurrent");
	holder<syncBarrier> barrier = newSyncBarrier(4);
	holder<syncMutex> mutex = newSyncMutex();
	holder<syncSemaphore> semaphore = newSyncSemaphore(1, 1);
	barrierGlobal = barrier.get();
	mutexGlobal = mutex.get();
	semaphoreGlobal = semaphore.get();

	{
		CAGE_TESTCASE("try lock mutex");
		for (uint32 i = 0; i < 3; i++)
		{
			if (auto lock = scopeLock<syncMutex>(mutex, true))
			{
				newThread(delegate<void()>().bind<&tryLockTest>(), string() + "try lock");
			}
			else
			{
				CAGE_TEST(false);
			}
		}
	}

	{
		CAGE_TESTCASE("barrier");
		holder<threadHandle> thrs[4];
		for (uint32 i = 0; i < 4; i++)
			thrs[i] = newThread(delegate<void()>().bind<&threadTest>(), string() + "worker_" + i);
		for (uint32 i = 0; i < 4; i++)
			thrs[i]->wait();
	}

	{
		CAGE_TESTCASE("thread-pool");
		{
			CAGE_TESTCASE("global variable (mutex)");
			counterGlobal = 0;
			holder<threadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&mutexTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			CAGE_TEST(counterGlobal == 600);
		}
		{
			CAGE_TESTCASE("global variable (semaphore)");
			counterGlobal = 0;
			holder<threadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&semaphoreTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			CAGE_TEST(counterGlobal == 600);
		}
		{
			CAGE_TESTCASE("thread local variable");
			counterGlobal = 0;
			counterLocal = 0;
			holder<threadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&barrierTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			thrs->function.bind<&barrierTestFinal>();
			thrs->run();
			CAGE_TEST(counterLocal == 0);
			CAGE_TEST(counterGlobal == 6);
		}
		{
			CAGE_TESTCASE("thread-pool run must block");
			counterGlobal = 0;
			holder<threadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&longTimeTest>();
			thrs->run();
			CAGE_TEST(counterGlobal == 40);
		}
	}
}
