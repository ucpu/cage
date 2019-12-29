#include "main.h"
#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>

namespace
{
	uint32 counterGlobal;
	thread_local uint32 counterLocal;
	Mutex *mutexGlobal;
	Semaphore *semaphoreGlobal;
	Barrier *barrierGlobal;

	void threadTest()
	{
		for (uint32 i = 0; i < 100; i++)
			ScopeLock<Barrier> l(barrierGlobal);
	}

	void mutexTest(uint32 idx, uint32)
	{
		ScopeLock<Mutex> l(mutexGlobal);
		counterGlobal += idx;
	}

	void semaphoreTest(uint32 idx, uint32)
	{
		ScopeLock<Semaphore> l(semaphoreGlobal);
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
			ScopeLock<Mutex> l(mutexGlobal);
			counterGlobal += idx;
		}
	}

	void longTimeTest(uint32 idx, uint32)
	{
		for (uint32 i = 0; i < 10; i++)
		{
			threadSleep(2000);
			{
				ScopeLock<Mutex> l(mutexGlobal);
				counterGlobal += 1;
			}
		}
	}

	void tryLockTest()
	{
		if (ScopeLock<Mutex>(mutexGlobal, true))
		{
			CAGE_TEST(false);
		}
	}
}

void testConcurrent()
{
	CAGE_TESTCASE("concurrent");
	Holder<Barrier> barrier = newSyncBarrier(4);
	Holder<Mutex> mutex = newSyncMutex();
	Holder<Semaphore> semaphore = newSyncSemaphore(1, 1);
	barrierGlobal = barrier.get();
	mutexGlobal = mutex.get();
	semaphoreGlobal = semaphore.get();

	{
		CAGE_TESTCASE("try lock mutex");
		for (uint32 i = 0; i < 3; i++)
		{
			if (auto lock = ScopeLock<Mutex>(mutex, true))
			{
				newThread(Delegate<void()>().bind<&tryLockTest>(), "try lock");
			}
			else
			{
				CAGE_TEST(false);
			}
		}
	}

	{
		CAGE_TESTCASE("barrier");
		Holder<Thread> thrs[4];
		for (uint32 i = 0; i < 4; i++)
			thrs[i] = newThread(Delegate<void()>().bind<&threadTest>(), stringizer() + "worker_" + i);
		for (uint32 i = 0; i < 4; i++)
			thrs[i]->wait();
	}

	{
		CAGE_TESTCASE("thread-pool");
		{
			CAGE_TESTCASE("global variable (mutex)");
			counterGlobal = 0;
			Holder<ThreadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&mutexTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			CAGE_TEST(counterGlobal == 600);
		}
		{
			CAGE_TESTCASE("global variable (semaphore)");
			counterGlobal = 0;
			Holder<ThreadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&semaphoreTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			CAGE_TEST(counterGlobal == 600);
		}
		{
			CAGE_TESTCASE("thread local variable");
			counterGlobal = 0;
			counterLocal = 0;
			Holder<ThreadPool> thrs = newThreadPool("worker_", 4);
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
			Holder<ThreadPool> thrs = newThreadPool("worker_", 4);
			thrs->function.bind<&longTimeTest>();
			thrs->run();
			CAGE_TEST(counterGlobal == 40);
		}
		{
			CAGE_TESTCASE("threadPoolTasksSplit");
			uint32 b, e;
			threadPoolTasksSplit(0, 3, 10, b, e);
			CAGE_TEST(b == 0 && e == 3);
			threadPoolTasksSplit(1, 3, 10, b, e);
			CAGE_TEST(b == 3 && e == 6);
			threadPoolTasksSplit(2, 3, 10, b, e);
			CAGE_TEST(b == 6 && e == 10);
			threadPoolTasksSplit(0, 0, 10, b, e);
			CAGE_TEST(b == 0 && e == 10);
		}
	}
}
