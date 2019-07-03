#include <atomic>

#include "main.h"
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/threadPool.h>

namespace
{
	class task
	{
	public:
		task(uint32 id, bool final = false) : id(id), final(final) {}
		uint32 id;
		bool final;
	};

	class tester
	{
	public:
		tester() : produceItems(1000), produceFinals(1)
		{
			concurrentQueueCreateConfig cfg;
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
			catch (const concurrentQueueTerminated &)
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
			catch (const concurrentQueueTerminated &)
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
			catch (const concurrentQueueTerminated &)
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
			catch (const concurrentQueueTerminated &)
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
		holder<concurrentQueue<task>> queue;
	};

	class testerPtr
	{
	public:
		testerPtr() : produceItems(1000), produceFinals(1)
		{
			concurrentQueueCreateConfig cfg;
			cfg.maxElements = 10;
			queue = newConcurrentQueue<task*>(cfg, delegate<void(task*)>().bind<testerPtr, &testerPtr::destroy>(this));
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
						break;
					else
						threadSleep(1); // simulate work
					destroy(tsk);
				}
			}
			catch (const concurrentQueueTerminated &)
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
					queue->push(tsk);
				}
				for (uint32 i = 0; i < produceFinals; i++)
				{
					threadSleep(1); // simulate work
					task *tsk = create(i + produceItems, true);
					queue->push(tsk);
				}
			}
			catch (const concurrentQueueTerminated &)
			{
				// nothing
			}
		}

		const uint32 produceItems;
		const uint32 produceFinals;
		holder<concurrentQueue<task*>> queue;
	};
}

void testConcurrentQueue()
{
	CAGE_TESTCASE("concurrentQueue");

	{
		CAGE_TESTCASE("single producer single consumer (blocking)");
		tester t;
		holder<threadHandle> t1 = newThread(delegate<void()>().bind<tester, &tester::consumeBlocking>(&t), "consumer");
		holder<threadHandle> t2 = newThread(delegate<void()>().bind<tester, &tester::produceBlocking>(&t), "producer");
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("single producer single consumer (polling)");
		tester t;
		holder<threadHandle> t1 = newThread(delegate<void()>().bind<tester, &tester::consumePolling>(&t), "consumer");
		holder<threadHandle> t2 = newThread(delegate<void()>().bind<tester, &tester::producePolling>(&t), "producer");
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("single producer single consumer (pointers) (blocking)");
		testerPtr t;
		holder<threadHandle> t1 = newThread(delegate<void()>().bind<testerPtr, &testerPtr::consumeBlocking>(&t), "consumer");
		holder<threadHandle> t2 = newThread(delegate<void()>().bind<testerPtr, &testerPtr::produceBlocking>(&t), "producer");
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("multiple producers multiple consumers (blocking)");
		tester t;
		holder<threadPool> t1 = newThreadPool("pool_", 6);
		t1->function = delegate<void(uint32, uint32)>().bind<tester, &tester::poolBlocking>(&t);
		t1->run();
	}

	{
		CAGE_TESTCASE("multiple producers multiple consumers (polling)");
		tester t;
		holder<threadPool> t1 = newThreadPool("pool_", 6);
		t1->function = delegate<void(uint32, uint32)>().bind<tester, &tester::poolPolling>(&t);
		t1->run();
	}

	{
		CAGE_TESTCASE("termination (blocking)");
		tester t;
		holder<threadHandle> t1 = newThread(delegate<void()>().bind<tester, &tester::consumeBlocking>(&t), "consumer");
		holder<threadHandle> t2 = newThread(delegate<void()>().bind<tester, &tester::produceBlocking>(&t), "producer");
		threadSleep(10);
		t.queue->terminate();
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("termination (polling)");
		tester t;
		holder<threadHandle> t1 = newThread(delegate<void()>().bind<tester, &tester::consumePolling>(&t), "consumer");
		holder<threadHandle> t2 = newThread(delegate<void()>().bind<tester, &tester::producePolling>(&t), "producer");
		threadSleep(10);
		t.queue->terminate();
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("termination (pointers) (blocking)");
		testerPtr t;
		holder<threadHandle> t1 = newThread(delegate<void()>().bind<testerPtr, &testerPtr::consumeBlocking>(&t), "consumer");
		holder<threadHandle> t2 = newThread(delegate<void()>().bind<testerPtr, &testerPtr::produceBlocking>(&t), "producer");
		threadSleep(10);
		t.queue->terminate();
		t1->wait();
		t2->wait();
	}
}
