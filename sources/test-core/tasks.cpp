#include "main.h"
#include <cage-core/tasks.h>
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
			CAGE_THROW_ERROR(Exception, "task is intentionally broken");
		}
	}

	void testTasksWait()
	{
		CAGE_TESTCASE("blocking");

		{
			CAGE_TESTCASE("array, function");
			TaskTester arr[10];
			tasksRun<TaskTester>(Delegate<void(TaskTester &tester)>().bind<&testerRun>(), arr);
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("array, operator()");
			TaskTester arr[10];
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
			CAGE_TESTCASE("exception");
			CAGE_TEST_THROWN(tasksRun(Delegate<void(uint32)>().bind<&throwingTasks>(), 20)); // one exception
			CAGE_TEST_THROWN(tasksRun(Delegate<void(uint32)>().bind<&throwingTasks>(), 50)); // two exceptions
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
			Holder<PointerRange<TaskTester>> arr = newTaskTesterArray(10);
			tasksRunAsync<TaskTester>(Delegate<void(TaskTester &tester)>().bind<&testerRun>(), arr.share())->wait();
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("array, operator()");
			Holder<PointerRange<TaskTester>> arr = newTaskTesterArray(10);
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
			CAGE_TESTCASE("exception");
			{
				Holder<detail::AsyncTask> ref = tasksRunAsync(Delegate<void(uint32)>().bind<&throwingTasks>(), 20); // one exception
				CAGE_TEST_THROWN(ref->wait());
			}
			{
				Holder<detail::AsyncTask> ref = tasksRunAsync(Delegate<void(uint32)>().bind<&throwingTasks>(), 50); // two exceptions
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
}

void testTasks()
{
	CAGE_TESTCASE("tasks");
	//for (uint32 i = 0; i < 100; i++)
	{
		testTasksWait();
		testTasksAsync();
		testTasksRecursive();
	}
}
