#include <atomic>

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/threadPool.h>

namespace
{
	std::atomic<int> itemsCounter;

	class Task
	{
	public:
		Task(uint32 id, bool final = false) : id(id), final(final) { itemsCounter++; }

		Task(const Task &other) : id(other.id), final(other.final) { itemsCounter++; }

		Task(Task &&other) noexcept : id(other.id), final(other.final) { itemsCounter++; }

		Task &operator=(const Task &other)
		{
			id = other.id;
			final = other.final;
			// items count does not change
			return *this;
		}

		Task &operator=(Task &&other) noexcept
		{
			id = other.id;
			final = other.final;
			// items count does not change
			return *this;
		}

		~Task()
		{
			try
			{
				CAGE_TEST(itemsCounter > 0);
			}
			catch (...)
			{
				detail::irrecoverableError("exception in ~Task (in testing)");
			}
			itemsCounter--;
		}

		uint32 id;
		bool final;
	};

	class Tester
	{
	public:
		Tester() : produceItems(1000), produceFinals(1), queue(10) {}

		void consumeBlocking()
		{
			try
			{
				while (true)
				{
					Task tsk(-1);
					queue.pop(tsk);
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
					Task tsk(-1);
					if (queue.tryPop(tsk))
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
					Task tsk(i);
					queue.push(tsk);
				}
				for (uint32 i = 0; i < produceFinals; i++)
				{
					threadSleep(1); // simulate work
					Task tsk(i + produceItems, true);
					queue.push(tsk);
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
					Task tsk(i);
					while (!queue.tryPush(tsk))
						threadSleep(1);
				}
				for (uint32 i = 0; i < produceFinals; i++)
				{
					threadSleep(1); // simulate work
					Task tsk(i + produceItems, true);
					while (!queue.tryPush(tsk))
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
		ConcurrentQueue<Task> queue;
	};
}

void testConcurrentQueue()
{
	CAGE_TESTCASE("ConcurrentQueue");
	CAGE_TEST(itemsCounter == 0); // sanity check

	{
		CAGE_TESTCASE("single producer single consumer (blocking)");
		Tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<Tester, &Tester::consumeBlocking>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<Tester, &Tester::produceBlocking>(&t), "producer");
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("single producer single consumer (polling)");
		Tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<Tester, &Tester::consumePolling>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<Tester, &Tester::producePolling>(&t), "producer");
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("multiple producers multiple consumers (blocking)");
		Tester t;
		Holder<ThreadPool> t1 = newThreadPool("pool_", 6);
		t1->function = Delegate<void(uint32, uint32)>().bind<Tester, &Tester::poolBlocking>(&t);
		t1->run();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("multiple producers multiple consumers (polling)");
		Tester t;
		Holder<ThreadPool> t1 = newThreadPool("pool_", 6);
		t1->function = Delegate<void(uint32, uint32)>().bind<Tester, &Tester::poolPolling>(&t);
		t1->run();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("termination (blocking)");
		Tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<Tester, &Tester::consumeBlocking>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<Tester, &Tester::produceBlocking>(&t), "producer");
		threadSleep(10);
		t.queue.terminate();
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);

	{
		CAGE_TESTCASE("termination (polling)");
		Tester t;
		Holder<Thread> t1 = newThread(Delegate<void()>().bind<Tester, &Tester::consumePolling>(&t), "consumer");
		Holder<Thread> t2 = newThread(Delegate<void()>().bind<Tester, &Tester::producePolling>(&t), "producer");
		threadSleep(10);
		t.queue.terminate();
		t1->wait();
		t2->wait();
	}
	CAGE_TEST(itemsCounter == 0);
}
