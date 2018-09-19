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

	typedef concurrentQueueClass<task> queueType;

	class tester
	{
	public:
		tester(queueType *queue) : queue(queue), produceItems(1000), produceFinals(1) {}

		void consumeBlocking()
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

		void consumePolling()
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

		void produceBlocking()
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

		void producePolling()
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

		uint32 produceItems;
		uint32 produceFinals;
		queueType *queue;
	};
}

void testConcurrentQueue()
{
	CAGE_TESTCASE("concurrentQueue");

	concurrentQueueCreateConfig cfg;
	cfg.maxElements = 10;

	{
		CAGE_TESTCASE("single producer single consumer (blocking)");
		holder<concurrentQueueClass<task>> q = newConcurrentQueue<task>(cfg);
		tester t(q.get());
		holder<threadClass> t1 = newThread(delegate<void()>().bind<tester, &tester::consumeBlocking>(&t), "consumer");
		holder<threadClass> t2 = newThread(delegate<void()>().bind<tester, &tester::produceBlocking>(&t), "producer");
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("single producer single consumer (polling)");
		holder<concurrentQueueClass<task>> q = newConcurrentQueue<task>(cfg);
		tester t(q.get());
		holder<threadClass> t1 = newThread(delegate<void()>().bind<tester, &tester::consumePolling>(&t), "consumer");
		holder<threadClass> t2 = newThread(delegate<void()>().bind<tester, &tester::producePolling>(&t), "producer");
		t1->wait();
		t2->wait();
	}

	{
		CAGE_TESTCASE("multiple producers multiple consumers (blocking)");
		holder<concurrentQueueClass<task>> q = newConcurrentQueue<task>(cfg);
		tester t(q.get());
		holder<threadPoolClass> t1 = newThreadPool("pool_", 6);
		t1->function = delegate<void(uint32,uint32)>().bind<tester, &tester::poolBlocking>(&t);
		t1->run();
	}

	{
		CAGE_TESTCASE("multiple producers multiple consumers (polling)");
		holder<concurrentQueueClass<task>> q = newConcurrentQueue<task>(cfg);
		tester t(q.get());
		holder<threadPoolClass> t1 = newThreadPool("pool_", 6);
		t1->function = delegate<void(uint32,uint32)>().bind<tester, &tester::poolPolling>(&t);
		t1->run();
	}
}
