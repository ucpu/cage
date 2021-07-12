#include "main.h"
#include <cage-core/tasks.h>
#include <cage-core/concurrent.h>
#include <cage-core/pointerRangeHolder.h>

#include <atomic>

namespace
{
	struct TaskTester : private Noncopyable
	{
		std::atomic<sint32> runCounter = 0;
		std::atomic<sint32> invocationsSum = 0;

		TaskTester() = default;

		TaskTester(TaskTester &&) noexcept
		{
			// nothing
		}

		TaskTester &operator = (TaskTester &&) noexcept
		{
			// nothing
			return *this;
		}

		void operator() ()
		{
			runCounter++;
		}

		void operator() (uint32 idx)
		{
			runCounter++;
			invocationsSum += idx;
		}
	};

	void testerRun(TaskTester &tester)
	{
		tester.runCounter++;
	}

	void testerRun(TaskTester &tester, uint32 idx)
	{
		tester.runCounter++;
		tester.invocationsSum += idx;
	}

	void throwingTasks(uint32 idx)
	{
		if (idx == 13 || idx == 42)
		{
			detail::OverrideBreakpoint ob;
			CAGE_THROW_ERROR(Exception, "intentionally throwing task");
		}
	}

	void testTasksWait()
	{
		CAGE_TESTCASE("blocking");

		{
			CAGE_TESTCASE("array, function");
			TaskTester arr[20];
			tasksRun<TaskTester>(Delegate<void(TaskTester &tester)>().bind<&testerRun>(), arr);
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("array, operator()");
			TaskTester arr[20];
			tasksRun<TaskTester>(arr);
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("single, function");
			TaskTester data;
			tasksRun<TaskTester>(Delegate<void(TaskTester &tester, uint32)>().bind<&testerRun>(), data, 5);
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("single, operator()");
			TaskTester data;
			tasksRun<TaskTester>(data, 5);
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("just delegate");
			TaskTester data;
			tasksRun(Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 5);
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("zero invocations");
			TaskTester data;
			tasksRun(Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 0);
			CAGE_TEST(data.runCounter == 0);
			CAGE_TEST(data.invocationsSum == 0);
		}

		{
			CAGE_TESTCASE("exception");
			CAGE_TEST_THROWN(tasksRun(Delegate<void(uint32)>().bind<&throwingTasks>(), 30)); // one exception
			CAGE_TEST_THROWN(tasksRun(Delegate<void(uint32)>().bind<&throwingTasks>(), 60)); // two exceptions
		}
	}

	Holder<PointerRange<TaskTester>> newTaskTesterArray(uint32 cnt)
	{
		PointerRangeHolder<TaskTester> arr;
		arr.resize(cnt);
		return arr;
	}

	void testTasksAsync()
	{
		CAGE_TESTCASE("async");

		{
			CAGE_TESTCASE("array, function");
			Holder<PointerRange<TaskTester>> arr = newTaskTesterArray(20);
			tasksRunAsync<TaskTester>(Delegate<void(TaskTester &tester)>().bind<&testerRun>(), arr.share())->wait();
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("array, operator()");
			Holder<PointerRange<TaskTester>> arr = newTaskTesterArray(20);
			tasksRunAsync<TaskTester>(arr.share())->wait();
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("single, function");
			Holder<TaskTester> data = systemMemory().createHolder<TaskTester>();
			tasksRunAsync<TaskTester>(Delegate<void(TaskTester &tester, uint32)>().bind<&testerRun>(), data.share(), 5)->wait();
			CAGE_TEST(data->runCounter == 5);
			CAGE_TEST(data->invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("single, operator()");
			Holder<TaskTester> data = systemMemory().createHolder<TaskTester>();
			tasksRunAsync<TaskTester>(data.share(), 5)->wait();
			CAGE_TEST(data->runCounter == 5);
			CAGE_TEST(data->invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("just delegate");
			TaskTester data;
			tasksRunAsync(Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 5)->wait();
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("zero invocations");
			TaskTester data;
			tasksRunAsync(Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 0)->wait();
			CAGE_TEST(data.runCounter == 0);
			CAGE_TEST(data.invocationsSum == 0);
		}

		{
			CAGE_TESTCASE("exception");
			{
				Holder<detail::AsyncTask> ref = tasksRunAsync(Delegate<void(uint32)>().bind<&throwingTasks>(), 30); // one exception
				CAGE_TEST_THROWN(ref->wait());
			}
			{
				Holder<detail::AsyncTask> ref = tasksRunAsync(Delegate<void(uint32)>().bind<&throwingTasks>(), 60); // two exceptions
				CAGE_TEST_THROWN(ref->wait());
			}
		}
	}

	struct RecursiveTester : private Immovable
	{
		static inline std::atomic<sint32> instances = 0;
		static inline std::atomic<sint32> runCounter = 0;

		const uint32 depth = 0;

		RecursiveTester(uint32 depth) : depth(depth)
		{
			instances++;
		}

		~RecursiveTester()
		{
			instances--;
		}

		void operator()(uint32 = 0)
		{
			runCounter++;
			if (depth < 5)
			{
				RecursiveTester insts[2] = { { depth + 1 }, { depth + 1 } };
				tasksRun<RecursiveTester>(insts);
			}
		}
	};

	void testTasksRecursive()
	{
		CAGE_TESTCASE("recursive");
		RecursiveTester::instances = 0;
		RecursiveTester::runCounter = 0;
		RecursiveTester tester(0);
		CAGE_TEST(RecursiveTester::instances == 1);
		CAGE_TEST(RecursiveTester::runCounter == 0);
		tasksRun<RecursiveTester>(tester, 1);
		CAGE_TEST(RecursiveTester::instances == 1);
		CAGE_TEST(RecursiveTester::runCounter == 1 + 2 + 4 + 8 + 16 + 32);
	}

	struct ParallelTester
	{
		Holder<Barrier> bar = newBarrier(processorsCount());

		void operator()(uint32)
		{
			ScopeLock lock(bar);
		}
	};

	void testTasksAreParallel()
	{
		CAGE_TESTCASE("parallel");
		ParallelTester tester;
		tasksRun<ParallelTester>(tester, processorsCount() * 3);
	}

	void testTasksSplit()
	{
		CAGE_TESTCASE("tasksSplit");
		uint32 b, e;
		{
			CAGE_TESTCASE("zero threads");
			{
				auto [b, e] = tasksSplit(0, 0, 10);
				CAGE_TEST(b == 0 && e == 10);
			}
		}
		{
			CAGE_TESTCASE("1 thread (5 tasks)");
			{
				auto [b, e] = tasksSplit(0, 1, 5);
				CAGE_TEST(b == 0 && e == 5); // 5
			}
		}
		{
			CAGE_TESTCASE("2 threads (4 tasks)");
			{
				auto [b, e] = tasksSplit(0, 2, 8);
				CAGE_TEST(b == 0 && e == 4); // 4
			}
			{
				auto [b, e] = tasksSplit(1, 2, 8);
				CAGE_TEST(b == 4 && e == 8); // 4
			}
		}
		{
			CAGE_TESTCASE("2 threads (5 tasks)");
			{
				auto [b, e] = tasksSplit(0, 2, 5);
				CAGE_TEST(b == 0 && e == 2); // 2
			}
			{
				auto [b, e] = tasksSplit(1, 2, 5);
				CAGE_TEST(b == 2 && e == 5); // 3
			}
		}
		{
			CAGE_TESTCASE("3 threads (1 task)");
			{
				auto [b, e] = tasksSplit(0, 3, 1);
				CAGE_TEST(b == 0 && e == 0); // 0
			}
			{
				auto [b, e] = tasksSplit(1, 3, 1);
				CAGE_TEST(b == 0 && e == 0); // 0
			}
			{
				auto [b, e] = tasksSplit(2, 3, 1);
				CAGE_TEST(b == 0 && e == 1); // 1
			}
		}
		{
			CAGE_TESTCASE("3 threads (8 tasks)");
			{
				auto [b, e] = tasksSplit(0, 3, 8);
				CAGE_TEST(b == 0 && e == 2); // 2
			}
			{
				auto [b, e] = tasksSplit(1, 3, 8);
				CAGE_TEST(b == 2 && e == 5); // 3
			}
			{
				auto [b, e] = tasksSplit(2, 3, 8);
				CAGE_TEST(b == 5 && e == 8); // 3
			}
		}
		{
			CAGE_TESTCASE("3 threads (10 tasks)");
			{
				auto [b, e] = tasksSplit(0, 3, 10);
				CAGE_TEST(b == 0 && e == 3); // 3
			}
			{
				auto [b, e] = tasksSplit(1, 3, 10);
				CAGE_TEST(b == 3 && e == 6); // 3
			}
			{
				auto [b, e] = tasksSplit(2, 3, 10);
				CAGE_TEST(b == 6 && e == 10); // 4
			}
		}
		{
			CAGE_TESTCASE("3 threads (12 tasks)");
			{
				auto [b, e] = tasksSplit(0, 3, 12);
				CAGE_TEST(b == 0 && e == 4); // 4
			}
			{
				auto [b, e] = tasksSplit(1, 3, 12);
				CAGE_TEST(b == 4 && e == 8); // 4
			}
			{
				auto [b, e] = tasksSplit(2, 3, 12);
				CAGE_TEST(b == 8 && e == 12); // 4
			}
		}
		{
			CAGE_TESTCASE("4 threads (21 tasks)");
			{
				auto [b, e] = tasksSplit(0, 4, 21);
				CAGE_TEST(b == 0 && e == 5); // 5
			}
			{
				auto [b, e] = tasksSplit(1, 4, 21);
				CAGE_TEST(b == 5 && e == 10); // 5
			}
			{
				auto [b, e] = tasksSplit(2, 4, 21);
				CAGE_TEST(b == 10 && e == 15); // 5
			}
			{
				auto [b, e] = tasksSplit(3, 4, 21);
				CAGE_TEST(b == 15 && e == 21); // 6
			}
		}
		{
			CAGE_TESTCASE("4 threads (23 tasks)");
			{
				auto [b, e] = tasksSplit(0, 4, 23);
				CAGE_TEST(b == 0 && e == 5); // 5
			}
			{
				auto [b, e] = tasksSplit(1, 4, 23);
				CAGE_TEST(b == 5 && e == 11); // 6
			}
			{
				auto [b, e] = tasksSplit(2, 4, 23);
				CAGE_TEST(b == 11 && e == 17); // 6
			}
			{
				auto [b, e] = tasksSplit(3, 4, 23);
				CAGE_TEST(b == 17 && e == 23); // 6
			}
		}
		{
			CAGE_TESTCASE("5 threads (2 tasks)");
			{
				auto [b, e] = tasksSplit(0, 5, 2);
				CAGE_TEST(b == 0 && e == 0); // 0
			}
			{
				auto [b, e] = tasksSplit(1, 5, 2);
				CAGE_TEST(b == 0 && e == 0); // 0
			}
			{
				auto [b, e] = tasksSplit(2, 5, 2);
				CAGE_TEST(b == 0 && e == 0); // 0
			}
			{
				auto [b, e] = tasksSplit(3, 5, 2);
				CAGE_TEST(b == 0 && e == 1); // 1
			}
			{
				auto [b, e] = tasksSplit(4, 5, 2);
				CAGE_TEST(b == 1 && e == 2); // 1
			}
		}
		{
			CAGE_TESTCASE("5 threads (4 tasks)");
			{
				auto [b, e] = tasksSplit(0, 5, 4);
				CAGE_TEST(b == 0 && e == 0); // 0
			}
			{
				auto [b, e] = tasksSplit(1, 5, 4);
				CAGE_TEST(b == 0 && e == 1); // 1
			}
			{
				auto [b, e] = tasksSplit(2, 5, 4);
				CAGE_TEST(b == 1 && e == 2); // 1
			}
			{
				auto [b, e] = tasksSplit(3, 5, 4);
				CAGE_TEST(b == 2 && e == 3); // 1
			}
			{
				auto [b, e] = tasksSplit(4, 5, 4);
				CAGE_TEST(b == 3 && e == 4); // 1
			}
		}
	}
}

void testTasks()
{
	CAGE_TESTCASE("tasks");
	testTasksWait();
	testTasksAsync();
	testTasksRecursive();
	testTasksAreParallel();
	testTasksSplit();
}
