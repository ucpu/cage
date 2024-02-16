#include <algorithm>
#include <atomic>
#include <vector>

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/math.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/tasks.h>
#include <cage-core/timer.h>

namespace
{
	void someMeaninglessWork()
	{
		std::atomic<sint32> v = 0;
		for (uint32 i = 0; i < 1000; i++)
			v += i;
	}

	struct TaskTester : private Noncopyable
	{
		std::atomic<sint32> runCounter = 0;
		std::atomic<sint32> invocationsSum = 0;

		TaskTester() = default;

		TaskTester(TaskTester &&) noexcept
		{
			// nothing
		}

		TaskTester &operator=(TaskTester &&) noexcept
		{
			// nothing
			return *this;
		}

		void operator()()
		{
			someMeaninglessWork();
			runCounter++;
		}

		void operator()(uint32 idx)
		{
			someMeaninglessWork();
			runCounter++;
			invocationsSum += idx;
		}
	};

	void testerRun(TaskTester &tester)
	{
		someMeaninglessWork();
		tester.runCounter++;
	}

	void testerRun(TaskTester &tester, uint32 idx)
	{
		someMeaninglessWork();
		tester.runCounter++;
		tester.invocationsSum += idx;
	}

	void throwingTasks(uint32 idx)
	{
		someMeaninglessWork();
		if (idx == 13 || idx == 42)
		{
			detail::OverrideBreakpoint ob;
			CAGE_THROW_ERROR(Exception, "intentionally throwing task");
		}
	}

	void testTasksBlocking()
	{
		CAGE_TESTCASE("blocking");

		{
			CAGE_TESTCASE("array, function");
			TaskTester arr[20];
			tasksRunBlocking<TaskTester>("blocking", Delegate<void(TaskTester & tester)>().bind<&testerRun>(), arr);
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("array, operator()");
			TaskTester arr[20];
			tasksRunBlocking<TaskTester>("blocking", arr);
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("single, function");
			TaskTester data;
			tasksRunBlocking<TaskTester>("blocking", Delegate<void(TaskTester & tester, uint32)>().bind<&testerRun>(), data, 5);
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("single, operator()");
			TaskTester data;
			tasksRunBlocking<TaskTester>("blocking", data, 5);
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("just delegate");
			TaskTester data;
			tasksRunBlocking("blocking", Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 5);
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("zero invocations");
			TaskTester data;
			tasksRunBlocking("blocking", Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 0);
			CAGE_TEST(data.runCounter == 0);
			CAGE_TEST(data.invocationsSum == 0);
		}

		{
			CAGE_TESTCASE("one exception");
			CAGE_TEST_THROWN(tasksRunBlocking("blocking", Delegate<void(uint32)>().bind<&throwingTasks>(), 30));
		}

		{
			CAGE_TESTCASE("two exceptions");
			CAGE_TEST_THROWN(tasksRunBlocking("blocking", Delegate<void(uint32)>().bind<&throwingTasks>(), 60));
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
			tasksRunAsync<TaskTester>("async", Delegate<void(TaskTester & tester)>().bind<&testerRun>(), arr.share())->wait();
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("array, operator()");
			Holder<PointerRange<TaskTester>> arr = newTaskTesterArray(20);
			tasksRunAsync<TaskTester>("async", arr.share())->wait();
			for (TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("single, function");
			Holder<TaskTester> data = systemMemory().createHolder<TaskTester>();
			tasksRunAsync<TaskTester>("async", Delegate<void(TaskTester & tester, uint32)>().bind<&testerRun>(), data.share(), 5)->wait();
			CAGE_TEST(data->runCounter == 5);
			CAGE_TEST(data->invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("single, operator()");
			Holder<TaskTester> data = systemMemory().createHolder<TaskTester>();
			tasksRunAsync<TaskTester>("async", data.share(), 5)->wait();
			CAGE_TEST(data->runCounter == 5);
			CAGE_TEST(data->invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("just delegate");
			TaskTester data;
			tasksRunAsync("async", Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 5)->wait();
			CAGE_TEST(data.runCounter == 5);
			CAGE_TEST(data.invocationsSum == 0 + 1 + 2 + 3 + 4);
		}

		{
			CAGE_TESTCASE("zero invocations");
			TaskTester data;
			tasksRunAsync("async", Delegate<void(uint32)>().bind<TaskTester, &TaskTester::operator()>(&data), 0)->wait();
			CAGE_TEST(data.runCounter == 0);
			CAGE_TEST(data.invocationsSum == 0);
		}

		{
			CAGE_TESTCASE("exception");
			{
				Holder<AsyncTask> ref = tasksRunAsync("async", Delegate<void(uint32)>().bind<&throwingTasks>(), 30); // one exception
				CAGE_TEST_THROWN(ref->wait());
			}
			{
				Holder<AsyncTask> ref = tasksRunAsync("async", Delegate<void(uint32)>().bind<&throwingTasks>(), 60); // two exceptions
				CAGE_TEST_THROWN(ref->wait());
			}
		}

		{
			CAGE_TESTCASE("repeated waits");
			{
				CAGE_TESTCASE("non-throwing");
				Holder<AsyncTask> tsk = tasksRunAsync("async", Delegate<void(uint32)>().bind<&throwingTasks>(), 10); // no exception
				tsk->wait();
				tsk->wait();
				tsk->wait();
			}
			{
				CAGE_TESTCASE("throwing");
				Holder<AsyncTask> tsk = tasksRunAsync("async", Delegate<void(uint32)>().bind<&throwingTasks>(), 30); // one exception
				CAGE_TEST_THROWN(tsk->wait());
				CAGE_TEST_THROWN(tsk->wait());
				CAGE_TEST_THROWN(tsk->wait());
			}
		}
	}

	struct RecursiveTester : private Immovable
	{
		static inline std::atomic<sint32> instances = 0;
		static inline std::atomic<sint32> runCounter = 0;

		const uint32 depth = 0;

		RecursiveTester(uint32 depth) : depth(depth) { instances++; }

		~RecursiveTester() { instances--; }

		void operator()(uint32 = 0)
		{
			someMeaninglessWork();
			runCounter++;
			if (depth < 5)
			{
				RecursiveTester insts[2] = { { depth + 1 }, { depth + 1 } };
				tasksRunBlocking<RecursiveTester>("recursive", insts);
			}
		}
	};

	void testTasksRecursive()
	{
		CAGE_TESTCASE("recursive");
		RecursiveTester::instances = 0; // make the test repeatable
		RecursiveTester::runCounter = 0;
		RecursiveTester tester(0);
		CAGE_TEST(RecursiveTester::instances == 1);
		CAGE_TEST(RecursiveTester::runCounter == 0);
		tasksRunBlocking<RecursiveTester>("recursive", tester, 1);
		CAGE_TEST(RecursiveTester::instances == 1);
		CAGE_TEST(RecursiveTester::runCounter == 1 + 2 + 4 + 8 + 16 + 32);
	}

	struct ParallelTester : private Immovable
	{
		static inline std::atomic<sint32> counter = 0;
		std::atomic<sint32> runs = 0;

		Holder<Barrier> bar = newBarrier(processorsCount());

		ParallelTester() { counter++; }

		~ParallelTester() { counter--; }

		void operator()(uint32)
		{
			someMeaninglessWork();
			// note that using thread synchronization inside tasks is BAD practice
			// implementation is specifically designed for running tasks inside tasks, but other blocking operations may deadlock all runners
			ScopeLock lock(bar);
			runs++;
		}
	};

	// a tasking thread may mark a task completed and then context-switch before releasing the holder,
	// therefore we cannot immediately check that the counter is zero, and have to wait instead
	void waitForNoParallelTesters()
	{
		while (ParallelTester::counter != 0)
			threadYield();
	}

	void testTasksAreParallel()
	{
		CAGE_TESTCASE("parallel");
		waitForNoParallelTesters();
		{
			CAGE_TESTCASE("blocking");
			ParallelTester tester;
			tasksRunBlocking<ParallelTester>("parallel", tester, processorsCount() * 3);
			CAGE_TEST(tester.runs == processorsCount() * 3);
		}
		waitForNoParallelTesters();
		{
			CAGE_TESTCASE("async");
			Holder<ParallelTester> tester = systemMemory().createHolder<ParallelTester>();
			std::vector<Holder<AsyncTask>> tasks;
			tasks.resize(processorsCount() * 3);
			for (auto &it : tasks)
				it = tasksRunAsync("parallel", tester.share());
			for (auto &it : tasks)
				it->wait();
			CAGE_TEST(tester->runs == processorsCount() * 3);
		}
		waitForNoParallelTesters();
	}

	void testTasksSplit()
	{
		CAGE_TESTCASE("tasksSplit");
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

	struct TaskHolderTester : private Immovable
	{
		static inline std::atomic<sint32> counter = 0;
		std::atomic<sint32> runs = 0;

		TaskHolderTester()
		{
			CAGE_LOG(SeverityEnum::Info, "test", "test");
			counter++;
		}

		~TaskHolderTester() { counter--; }

		void operator()(uint32)
		{
			someMeaninglessWork();
			runs++;
		}
	};

	// a tasking thread may mark a task completed and then context-switch before releasing the holder,
	// therefore we cannot immediately check that the counter is zero, and have to wait instead
	void waitForNoHolderTesters()
	{
		while (TaskHolderTester::counter != 0)
			threadYield();
	}

	void testTasksHolders()
	{
		CAGE_TESTCASE("holders");
		waitForNoHolderTesters();
		{
			CAGE_TESTCASE("waiting");
			Holder<TaskHolderTester> t = systemMemory().createHolder<TaskHolderTester>();
			CAGE_TEST(TaskHolderTester::counter == 1);
			CAGE_TEST(t->runs == 0);
			Holder<AsyncTask> a = tasksRunAsync("holders", t.share(), 13);
			CAGE_TEST(TaskHolderTester::counter == 1);
			a->wait();
			CAGE_TEST(t->runs == 13);
		}
		waitForNoHolderTesters();
		{
			CAGE_TESTCASE("repeated waits");
			Holder<TaskHolderTester> t = systemMemory().createHolder<TaskHolderTester>();
			CAGE_TEST(TaskHolderTester::counter == 1);
			CAGE_TEST(t->runs == 0);
			Holder<AsyncTask> a = tasksRunAsync("holders", t.share(), 13);
			CAGE_TEST(TaskHolderTester::counter == 1);
			a->wait();
			CAGE_TEST(t->runs == 13);
			a->wait();
			CAGE_TEST(t->runs == 13);
			a->wait();
			CAGE_TEST(t->runs == 13);
			CAGE_TEST(TaskHolderTester::counter == 1);
		}
		waitForNoHolderTesters();
		{
			CAGE_TESTCASE("unordered");
			std::vector<Holder<TaskHolderTester>> testers;
			std::vector<Holder<AsyncTask>> tasks;
			for (uint32 i = 0; i < 10; i++)
			{
				Holder<TaskHolderTester> t = systemMemory().createHolder<TaskHolderTester>();
				Holder<AsyncTask> a = tasksRunAsync("holders", t.share(), randomRange(0, 42));
				testers.push_back(std::move(t));
				tasks.push_back(std::move(a));
			}
			for (uint32 i = 0; i < 3; i++)
			{
				for (auto &it : testers)
					if (randomChance() < 0.3)
						it.clear();
				for (auto &it : tasks)
				{
					if (randomChance() < 0.3)
					{
						if (it)
							it->wait();
						it.clear();
					}
				}
			}
			for (auto &it : tasks)
				if (it)
					it->wait();
			tasks.clear();
			testers.clear();
		}
		waitForNoHolderTesters();
	}

	void testTasksAggregation()
	{
		CAGE_TESTCASE("aggregation");

		{
			CAGE_TESTCASE("blocking array");
			TaskTester arr[20];
			tasksRunBlocking<TaskTester, 3>("blocking", arr);
			for (const TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}

		{
			CAGE_TESTCASE("async array");
			Holder<PointerRange<TaskTester>> arr = newTaskTesterArray(20);
			tasksRunAsync<TaskTester, 3>("async", arr.share())->wait();
			for (const TaskTester &t : arr)
			{
				CAGE_TEST(t.runCounter == 1);
				CAGE_TEST(t.invocationsSum == 0);
			}
		}
	}

	void testPerformance()
	{
		CAGE_TESTCASE("performance (parallel merge sort)");

		struct Sorter
		{
			PointerRange<uint32> data;

			static void merge(const PointerRange<const uint32> a_, const PointerRange<const uint32> b_, const PointerRange<uint32> o_)
			{
				CAGE_ASSERT(a_.size() + b_.size() == o_.size());
				const uint32 *a = a_.begin();
				const uint32 *b = b_.begin();
				uint32 *o = o_.begin();
				while (o != o_.end())
				{
					if (a == a_.end())
						*o++ = *b++;
					else if (b == b_.end())
						*o++ = *a++;
					else if (*a <= *b)
						*o++ = *a++;
					else
						*o++ = *b++;
				}
				CAGE_ASSERT(a == a_.end());
				CAGE_ASSERT(b == b_.end());
			}

			void operator()()
			{
				if (data.size() > 30)
				{
					uint32 *const dataMid = data.begin() + data.size() / 2;
					Sorter s[2];
					s[0].data = { data.begin(), dataMid };
					s[1].data = { dataMid, data.end() };
					tasksRunBlocking<Sorter>("sorting", s);
					std::vector<uint32> tmp;
					tmp.resize(data.size());
					merge(s[0].data, s[1].data, tmp);
					std::copy(tmp.begin(), tmp.end(), data.begin());
				}
				else
				{
					std::sort(data.begin(), data.end());
				}
			}
		};

		std::vector<uint32> data;
		data.resize(5000);
		Sorter s;
		s.data = data;
		Holder<Timer> tmr = newTimer();
		uint64 durations[31];
		for (uint64 &duration : durations)
		{
			for (uint32 &it : data)
				it = randomRange(0u, 1000000u);
			CAGE_TEST(!std::is_sorted(data.begin(), data.end()));
			tmr->reset();
			s.operator()();
			duration = tmr->duration();
			CAGE_TEST(std::is_sorted(data.begin(), data.end()));
		}
		std::sort(std::begin(durations), std::end(durations));
		CAGE_LOG(SeverityEnum::Info, "tasks performance", Stringizer() + "parallel merge sort avg duration: " + durations[15] + " us"); // median
	}

	struct StressTester : private Immovable
	{
		static inline std::atomic<sint32> counter = 0;
		static inline std::atomic<uint64> nextId = 1;

		uint64 id = nextId++;
		char data[100] = {};

		StressTester()
		{
			counter++;
			char tmp = 13;
			for (char &it : data)
				it = tmp++;
		}

		~StressTester() { counter--; }

		void check()
		{
			char tmp = 13;
			for (char it : data)
				CAGE_TEST(it == tmp++);
		}

		void run()
		{
			check();
			if (counter < randomRange(10, 100))
			{
				if (randomRange(0, 100) < 50)
				{
					const uint32 cnt = randomRange(1, 5);
					std::vector<Holder<AsyncTask>> arr;
					for (uint32 i = 0; i < cnt; i++)
						arr.push_back(tasksRunAsync("stress", systemMemory().createHolder<StressTester>(), randomRange(1, 3)));
					for (auto &it : arr)
						it->wait();
					arr.clear();
				}
				else
				{
					StressTester arr[3];
					tasksRunBlocking<StressTester>("stress", arr);
				}
			}
			check();
		}

		void operator()(uint32) { run(); }
		void operator()() { run(); }
	};

	void randomizedStressTest()
	{
		CAGE_TESTCASE("randomized stress test");
		{
			StressTester t;
			t();
		}
		while (StressTester::counter != 0)
			threadYield();
	}
}

void testTasks()
{
	CAGE_TESTCASE("tasks");
	testTasksBlocking();
	testTasksAsync();
	testTasksRecursive();
	testTasksAreParallel();
	testTasksSplit();
	testTasksHolders();
	testTasksAggregation();
	testPerformance();
	randomizedStressTest();
}
