#include <atomic>

#include "main.h"
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/threadPool.h>

namespace
{
	std::atomic<int> itemsCounter;

	class task
	{
	public:
		task(uint32 id, bool final = false) : id(id), final(final)
		{
			itemsCounter++;
		}

		task(const task &other) : id(other.id), final(other.final)
		{
			itemsCounter++;
		}

		task(task &&other) : id(other.id), final(other.final)
		{
			itemsCounter++;
		}

		task &operator = (const task &other)
		{
			id = other.id;
			final = other.final;
			// items count does not change
			return *this;
		}

		task &operator = (task &&other)
		{
			id = other.id;
			final = other.final;
			// items count does not change
			return *this;
		}

		~task()
		{
#ifdef _MSC_VER
#pragma warning( disable : 4297 ) // function assumed not to throw an exception but does
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wterminate"
#endif // _MSC_VER
			CAGE_TEST(itemsCounter > 0);
#ifdef _MSC_VER
#pragma warning( default : 4297 )
#else
#pragma GCC diagnostic pop
#endif // _MSC_VER
			itemsCounter--;
		}

		uint32 id;
		bool final;
	};

	class tester
	{
	public:
		tester() : produceItems(1000), produceFinals(1)
		{
			ConcurrentQueueCreateConfig cfg;
			cfg.maxElements = 10;
			queue = newConcurrentQueue<task>(cfg);
		}

		void consumeBlocking()
		{
			try
			{
				while (true)
				{
					task tsk(-1);
					queue->pop(tsk);
					if (tsk.final)
						break;
					else
						threadSleep(1); // simulate work
				}
			}
			catch (const ConcurrentQueueTerminated &)
			{
				// nothing
			}
		}

		void consumePolling()
		{
			try
			{
				while (true)
				{
					task tsk(-1);
					if (queue->tryPop(tsk))
					{
						if (tsk.final)
							break;
						else
							threadSleep(1); // simulate work
					}
					else
						threadSleep(1);
				}
			}
			catch (const ConcurrentQueueTerminated &)
			{
				// nothing
			}
		}

		void produceBlocking()
		{
			try
			{
				for (uint32 i = 0; i < produceItems; i++)
				{
					threadSleep(1); // simulate work
					task tsk(i);
					queue->push(tsk);
				}
				for (uint32 i = 0; i < produceFinals; i++)
				{
					threadSleep(1); // simulate work
					task tsk(i + produceItems, true);
					queue->push(tsk);
				}
			}
			catch (const ConcurrentQueueTerminated &)
			{
				// nothing
			}
		}

		void producePolling()
		{
			try
			{
				for (uint32 i = 0; i < produceItems; i++)
				{
					threadSleep(1); // simulate work
					task tsk(i);
					while (!queue->tryPush(tsk))
						threadSleep(1);
				}
				for (uint32 i = 0; i < produceFinals; i++)
				{
					threadSleep(1); // simulate work
					task tsk(i + produceItems, true);
					while (!queue->tryPush(tsk))
						threadSleep(1);
				}
			}
			catch (const ConcurrentQueueTerminated &)
			{
				// nothing
			}
		}

		void poolBlocking(uint32 a, uint32 b)
		{
			if (a % 2 == 0)
				produceBlocking();
			else
				consumeBlocking();
		}

		void poolPolling(uint32 a, uint32 b)
		{
			if (a % 2 == 0)
				producePolling();
			else
				consumePolling();
		}

		const uint32 produceItems;
		const uint32 produceFinals;
		Holder<ConcurrentQueue<task>> queue;
	};

	class testerPtr
	{
	public:
		testerPtr() : produceItems(1000), produceFinals(1)
		{
			ConcurrentQueueCreateConfig cfg;
			cfg.maxElements = 10;
			queue = newConcurrentQueue<task*>(cfg, Delegate<void(task*)>().bind<testerPtr, &testerPtr::destroy>(this));
		}

		task *create(uint32 i, bool final)
		{
			return detail::systemArena().createObject<task>(i, final);
		}

		void destroy(task *t)
		{
			detail::systemArena().destroy<task>(t);
		}

		void consumeBlocking()
		{
			try
			{
				while (true)
				{
					task *tsk = nullptr;
					queue->pop(tsk);
					if (tsk->final)
					{
						destroy(tsk);
						break;
					}
					else
						threadSleep(1); // simulate work
					destroy(tsk);
				}
			}
			catch (const ConcurrentQueueTerminated &)
			{
				// nothing
			}
		}

		void produceBlocking()
		{
			try
			{
				for (uint32 i = 0; i < produceItems; i++)
				{
					threadSleep(1); // simulate work
					task *tsk = create(i, false);
					try
					{
						queue->push(tsk);
					}
					catch (...)
					{
						destroy(tsk);
						throw;
					}
				}
				for (uint32 i = 0; i < produceFinals; i++)
				{
					threadSleep(1); // simulate work
					task *tsk = create(i + produceItems, true);
					try
					{
						queue->push(tsk);
					}
					catch (...)
					{
						destroy(tsk);
						throw;
					}
				}
			}
			catch (const ConcurrentQueueTerminated &)
			{
				// nothing
			}
		}

		const uint32 produceItems;
		const uint32 produceFinals;
		Holder<ConcurrentQueue<task*>> queue;
	};
}

void testConcurrentQueue()
{
	CAGE_TESTCASE("ConcurrentQueue");
	CAGE_TEST(itemsCounter == 0); // sanity check

	{
		CAGE_TESTCASE("single producer single consumer (blocking)");
		tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<tester, &tester::consumeBlocking>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<tester, &tester::produceBlocking>(&t), "producer");
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("single producer single consumer (polling)");
		tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<tester, &tester::consumePolling>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<tester, &tester::producePolling>(&t), "producer");
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("single producer single consumer (pointers) (blocking)");
		testerPtr t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<testerPtr, &testerPtr::consumeBlocking>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<testerPtr, &testerPtr::produceBlocking>(&t), "producer");
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("multiple producers multiple consumers (blocking)");
		tester t;
		Holder<ThreadPool> t1 = newThreadPool("pool_", 6);
		t1->function = Delegate<void(uint32, uint32)>().bind<tester, &tester::poolBlocking>(&t);
		t1->run();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("multiple producers multiple consumers (polling)");
		tester t;
		Holder<ThreadPool> t1 = newThreadPool("pool_", 6);
		t1->function = Delegate<void(uint32, uint32)>().bind<tester, &tester::poolPolling>(&t);
		t1->run();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("termination (blocking)");
		tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<tester, &tester::consumeBlocking>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<tester, &tester::produceBlocking>(&t), "producer");
		threadSleep(10);
		t.queue->terminate();
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("termination (polling)");
		tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<tester, &tester::consumePolling>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<tester, &tester::producePolling>(&t), "producer");
		threadSleep(10);
		t.queue->terminate();
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("termination (pointers) (blocking)");
		testerPtr t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<testerPtr, &testerPtr::consumeBlocking>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<testerPtr, &testerPtr::produceBlocking>(&t), "producer");
		threadSleep(10);
		t.queue->terminate();
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);
}
