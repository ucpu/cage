#include <algorithm>
#include <array>
#include <atomic>
#include <numeric>

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>
#include <cage-core/timer.h>

namespace
{
	uint32 counterGlobal = 0;
	thread_local uint32 counterLocal = 0;
	std::atomic<uint32> atomicGlobal = 0;
	Mutex *mutexGlobal;
	RwMutex *rwMutexGlobal;
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
			ScopeLock l(barrierGlobal);
	}

	void mutexTest(uint32 idx, uint32)
	{
		ScopeLock l(mutexGlobal);
		counterGlobal += idx;
	}

	void rwMutexTest(uint32 idx, uint32)
	{
		if (idx < 4)
		{ // writer

			ScopeLock l(rwMutexGlobal, WriteLockTag());
			counterGlobal += idx;
		}
		else
		{ // reader
			ScopeLock l(rwMutexGlobal, ReadLockTag());
			atomicGlobal += counterGlobal;
		}
	}

	void semaphoreTest(uint32 idx, uint32)
	{
		ScopeLock l(semaphoreGlobal);
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
			ScopeLock l(mutexGlobal);
			counterGlobal += idx;
		}
	}

	void longTimeTest(uint32 idx, uint32)
	{
		for (uint32 i = 0; i < 10; i++)
		{
			threadSleep(2000);
			{
				ScopeLock l(mutexGlobal);
				counterGlobal += 1;
			}
		}
	}

	void tryLockTest()
	{
		if (ScopeLock lock = ScopeLock(mutexGlobal, TryLockTag()))
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
	Holder<RwMutex> rwMutex = newRwMutex();
	Holder<Semaphore> semaphore = newSemaphore(1, 1);
	barrierGlobal = barrier.get();
	mutexGlobal = mutex.get();
	rwMutexGlobal = rwMutex.get();
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
			ok = String(e.message) == "intentionally thrown exception";
		}
		CAGE_TEST(ok);
	}

	{
		CAGE_TESTCASE("try lock mutex");
		for (uint32 i = 0; i < 3; i++)
		{
			if (auto lock = ScopeLock(mutex, TryLockTag()))
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
			thrs[i] = newThread(Delegate<void()>().bind<&threadTest>(), Stringizer() + "worker_" + i);
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
			CAGE_TESTCASE("global variable (rw-mutex)");
			counterGlobal = 0;
			Holder<ThreadPool> thrs = newThreadPool("worker_", 6);
			thrs->function.bind<&rwMutexTest>();
			for (uint32 i = 0; i < 100; i++)
				thrs->run();
			CAGE_TEST(counterGlobal == 600);
			// do not check atomicGlobal. its value cannot be determined due to order of threads
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
		CAGE_TESTCASE("sleeping precision");
		std::array<uint64, 10> frames = {};
		Holder<Timer> t = newTimer();
		for (uint64 &frame : frames)
		{
			const uint64 a = t->duration();
			constexpr uint64 period = 1'000'000 / 60;
			threadSleep(period);
			const uint64 b = t->duration();
			frame = b - a;
			CAGE_LOG(SeverityEnum::Info, "frame", Stringizer() + "frame: " + frame);
		}
		std::sort(frames.begin(), frames.end());
		uint64 sum = std::accumulate(frames.begin(), frames.end(), uint64(0));
		sum -= frames[0] + frames[frames.size() - 1]; // exclude extremes
		sum /= frames.size() - 2;
		CAGE_LOG(SeverityEnum::Info, "frame", Stringizer() + "average: " + sum);
		CAGE_LOG(SeverityEnum::Info, "frame", Stringizer() + "minimum: " + frames[1]);
		CAGE_LOG(SeverityEnum::Info, "frame", Stringizer() + "maximum: " + frames[frames.size() - 2]);
	}
}
