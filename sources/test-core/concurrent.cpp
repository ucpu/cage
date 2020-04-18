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

	void throwingFunction()
	{
		detail::OverrideBreakpoint ob;
		CAGE_THROW_ERROR(Exception, "intentionally thrown exception");
	}

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
		if (ScopeLock<Mutex>(mutexGlobal, TryLock()))
		{
			CAGE_TEST(false);
		}
	}
}

void testConcurrent()
{
	CAGE_TESTCASE("concurrent");
	Holder<Barrier> barrier = newBarrier(4);
	Holder<Mutex> mutex = newMutex();
	Holder<Semaphore> semaphore = newSemaphore(1, 1);
	barrierGlobal = barrier.get();
	mutexGlobal = mutex.get();
	semaphoreGlobal = semaphore.get();

	{
		CAGE_TESTCASE("thread with unhandled exception");
		bool ok = false;
		try
		{
			newThread(Delegate<void()>().bind<&throwingFunction>(), "throwing thread")->wait();
		}
		catch (const cage::Exception &e)
		{
			ok = string(e.message) == "intentionally thrown exception";
		}
		CAGE_TEST(ok);
	}

	{
		CAGE_TESTCASE("try lock mutex");
		for (uint32 i = 0; i < 3; i++)
		{
			if (auto lock = ScopeLock<Mutex>(mutex, TryLock()))
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
	}

	{
		CAGE_TESTCASE("threadPoolTasksSplit");
		uint32 b, e;
		{
			CAGE_TESTCASE("zero threads");
			threadPoolTasksSplit(0, 0, 10, b, e);
			CAGE_TEST(b == 0 && e == 10);
		}
		{
			CAGE_TESTCASE("1 thread (5 tasks)");
			threadPoolTasksSplit(0, 1, 5, b, e);
			CAGE_TEST(b == 0 && e == 5); // 5
		}
		{
			CAGE_TESTCASE("2 threads (4 tasks)");
			threadPoolTasksSplit(0, 2, 8, b, e);
			CAGE_TEST(b == 0 && e == 4); // 4
			threadPoolTasksSplit(1, 2, 8, b, e);
			CAGE_TEST(b == 4 && e == 8); // 4
		}
		{
			CAGE_TESTCASE("2 threads (5 tasks)");
			threadPoolTasksSplit(0, 2, 5, b, e);
			CAGE_TEST(b == 0 && e == 2); // 2
			threadPoolTasksSplit(1, 2, 5, b, e);
			CAGE_TEST(b == 2 && e == 5); // 3
		}
		{
			CAGE_TESTCASE("3 threads (1 task)");
			threadPoolTasksSplit(0, 3, 1, b, e);
			CAGE_TEST(b == 0 && e == 0); // 0
			threadPoolTasksSplit(1, 3, 1, b, e);
			CAGE_TEST(b == 0 && e == 0); // 0
			threadPoolTasksSplit(2, 3, 1, b, e);
			CAGE_TEST(b == 0 && e == 1); // 1
		}
		{
			CAGE_TESTCASE("3 threads (8 tasks)");
			threadPoolTasksSplit(0, 3, 8, b, e);
			CAGE_TEST(b == 0 && e == 2); // 2
			threadPoolTasksSplit(1, 3, 8, b, e);
			CAGE_TEST(b == 2 && e == 5); // 3
			threadPoolTasksSplit(2, 3, 8, b, e);
			CAGE_TEST(b == 5 && e == 8); // 3
		}
		{
			CAGE_TESTCASE("3 threads (10 tasks)");
			threadPoolTasksSplit(0, 3, 10, b, e);
			CAGE_TEST(b == 0 && e == 3); // 3
			threadPoolTasksSplit(1, 3, 10, b, e);
			CAGE_TEST(b == 3 && e == 6); // 3
			threadPoolTasksSplit(2, 3, 10, b, e);
			CAGE_TEST(b == 6 && e == 10); // 4
		}
		{
			CAGE_TESTCASE("3 threads (12 tasks)");
			threadPoolTasksSplit(0, 3, 12, b, e);
			CAGE_TEST(b == 0 && e == 4); // 4
			threadPoolTasksSplit(1, 3, 12, b, e);
			CAGE_TEST(b == 4 && e == 8); // 4
			threadPoolTasksSplit(2, 3, 12, b, e);
			CAGE_TEST(b == 8 && e == 12); // 4
		}
		{
			CAGE_TESTCASE("4 threads (21 tasks)");
			threadPoolTasksSplit(0, 4, 21, b, e);
			CAGE_TEST(b == 0 && e == 5); // 5
			threadPoolTasksSplit(1, 4, 21, b, e);
			CAGE_TEST(b == 5 && e == 10); // 5
			threadPoolTasksSplit(2, 4, 21, b, e);
			CAGE_TEST(b == 10 && e == 15); // 5
			threadPoolTasksSplit(3, 4, 21, b, e);
			CAGE_TEST(b == 15 && e == 21); // 6
		}
		{
			CAGE_TESTCASE("4 threads (23 tasks)");
			threadPoolTasksSplit(0, 4, 23, b, e);
			CAGE_TEST(b == 0 && e == 5); // 5
			threadPoolTasksSplit(1, 4, 23, b, e);
			CAGE_TEST(b == 5 && e == 11); // 6
			threadPoolTasksSplit(2, 4, 23, b, e);
			CAGE_TEST(b == 11 && e == 17); // 6
			threadPoolTasksSplit(3, 4, 23, b, e);
			CAGE_TEST(b == 17 && e == 23); // 6
		}
		{
			CAGE_TESTCASE("5 threads (2 tasks)");
			threadPoolTasksSplit(0, 5, 2, b, e);
			CAGE_TEST(b == 0 && e == 0); // 0
			threadPoolTasksSplit(1, 5, 2, b, e);
			CAGE_TEST(b == 0 && e == 0); // 0
			threadPoolTasksSplit(2, 5, 2, b, e);
			CAGE_TEST(b == 0 && e == 0); // 0
			threadPoolTasksSplit(3, 5, 2, b, e);
			CAGE_TEST(b == 0 && e == 1); // 1
			threadPoolTasksSplit(4, 5, 2, b, e);
			CAGE_TEST(b == 1 && e == 2); // 1
		}
		{
			CAGE_TESTCASE("5 threads (4 tasks)");
			threadPoolTasksSplit(0, 5, 4, b, e);
			CAGE_TEST(b == 0 && e == 0); // 0
			threadPoolTasksSplit(1, 5, 4, b, e);
			CAGE_TEST(b == 0 && e == 1); // 1
			threadPoolTasksSplit(2, 5, 4, b, e);
			CAGE_TEST(b == 1 && e == 2); // 1
			threadPoolTasksSplit(3, 5, 4, b, e);
			CAGE_TEST(b == 2 && e == 3); // 1
			threadPoolTasksSplit(4, 5, 4, b, e);
			CAGE_TEST(b == 3 && e == 4); // 1
		}
	}
}
