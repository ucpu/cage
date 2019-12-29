#include "main.h"
#include <cage-core/scheduler.h>
#include <cage-core/timer.h>
#include <cage-core/math.h> // randomRange
#include <cage-core/concurrent.h> // threadSleep

namespace
{
	void inc(uint32 *ptr)
	{
		(*ptr)++;
	}

	void incRandomSleep(uint32 *ptr)
	{
		(*ptr)++;
		threadSleep(randomRange(15000, 25000)); // avg 20 ms
	}

	void trigger(schedule *s)
	{
		s->trigger();
	}

	void stop(scheduler *s)
	{
		s->stop();
	}
}

void testScheduler()
{
	CAGE_TESTCASE("scheduler");

	{
		CAGE_TESTCASE("basics");
		holder<scheduler> sch = newScheduler({});
		uint32 periodicCount = 0;
		uint32 emptyCount = 0;
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &inc>(&periodicCount);
			c.name = "periodic add one";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Empty;
			c.action.bind<uint32*, &inc>(&emptyCount);
			c.name = "empty add one";
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Once;
			c.action.bind<scheduler*, &stop>(sch.get());
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		CAGE_TEST(periodicCount == 0);
		CAGE_TEST(emptyCount == 0);
		{
			holder<timer> tmr = newTimer();
			sch->run();
			uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(periodicCount >= 4 && periodicCount <= 8);
		CAGE_TEST(emptyCount >= 1 && emptyCount <= 15);
	}

	{
		CAGE_TESTCASE("two steady schedules with different periods");
		holder<scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "frequent";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt2);
			c.name = "infrequent";
			c.period = 80000;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Once;
			c.action.bind<scheduler*, &stop>(sch.get());
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			holder<timer> tmr = newTimer();
			sch->run();
			uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(cnt2 >= 1 && cnt2 <= 3);
	}

	{
		CAGE_TESTCASE("two steady schedules with different priorities");
		holder<scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "low priority";
			c.period = 30000;
			c.priority = 1;
			c.maxSteadyPeriods = m; // do not skip, we need to test that the higher priority is run more often
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt2);
			c.name = "high priority";
			c.period = 30000;
			c.priority = 3;
			c.maxSteadyPeriods = m;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Once;
			c.action.bind<scheduler*, &stop>(sch.get());
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			holder<timer> tmr = newTimer();
			sch->run();
			uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		uint32 sum = cnt1 + cnt2;
		CAGE_TEST(sum >= 6 && sum <= 14);
		CAGE_TEST(cnt2 > cnt1);
	}

	{
		CAGE_TESTCASE("steady and free periodic schedules");
		holder<scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "steady";
			c.period = 50000;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::FreePeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt2);
			c.name = "free";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Once;
			c.action.bind<scheduler*, &stop>(sch.get());
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			holder<timer> tmr = newTimer();
			sch->run();
			uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 2 && cnt1 <= 6);
		CAGE_TEST(cnt2 >= 2 && cnt2 <= 6);
	}

	{
		CAGE_TESTCASE("trigger external schedule");
		holder<scheduler> sch = newScheduler({});
		schedule *trig = nullptr;
		uint32 cnt1 = 0, cnt2 = 0;
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &inc>(&cnt1);
			c.name = "steady";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::External;
			c.action.bind<uint32*, &inc>(&cnt2);
			c.name = "external";
			trig = sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Once;
			c.action.bind<schedule*, &trigger>(trig);
			c.name = "trigger";
			c.delay = 100000;
			c.priority = 50;
			sch->newSchedule(c);
		}
		{
			scheduleCreateConfig c;
			c.type = scheduleTypeEnum::Once;
			c.action.bind<scheduler*, &stop>(sch.get());
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			holder<timer> tmr = newTimer();
			sch->run();
			uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(cnt2 == 1);
		CAGE_TEST(trig->runsCount() == 1);
	}
}
