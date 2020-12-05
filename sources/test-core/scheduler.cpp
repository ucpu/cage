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

	void trigger(Schedule *s)
	{
		s->trigger();
	}

	void stop(Scheduler *s)
	{
		s->stop();
	}
}

void testScheduler()
{
	CAGE_TESTCASE("Scheduler");

	{
		CAGE_TESTCASE("basics");
		Holder<Scheduler> sch = newScheduler({});
		uint32 periodicCount = 0;
		uint32 emptyCount = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &inc>(&periodicCount);
			c.name = "periodic add one";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Empty;
			c.action.bind<uint32*, &inc>(&emptyCount);
			c.name = "empty add one";
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler*, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		CAGE_TEST(periodicCount == 0);
		CAGE_TEST(emptyCount == 0);
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(periodicCount >= 4 && periodicCount <= 8);
		CAGE_TEST(emptyCount >= 1 && emptyCount <= 15);
	}

	{
		CAGE_TESTCASE("two steady schedules with different periods");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "frequent";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt2);
			c.name = "infrequent";
			c.period = 80000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler*, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(cnt2 >= 1 && cnt2 <= 3);
	}

	{
		CAGE_TESTCASE("two steady schedules with different priorities");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "low priority";
			c.period = 30000;
			c.priority = 1;
			c.maxSteadyPeriods = m; // do not skip, we need to test that the higher priority is run more often
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt2);
			c.name = "high priority";
			c.period = 30000;
			c.priority = 3;
			c.maxSteadyPeriods = m;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler*, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		const uint32 sum = cnt1 + cnt2;
		CAGE_TEST(sum >= 6 && sum <= 14);
		CAGE_TEST(cnt2 > cnt1);
	}

	{
		CAGE_TESTCASE("steady and free periodic schedules");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "steady";
			c.period = 50000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::FreePeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt2);
			c.name = "free";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler*, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 2 && cnt1 <= 6);
		CAGE_TEST(cnt2 >= 2 && cnt2 <= 6);
	}

	{
		CAGE_TESTCASE("trigger external schedule");
		Holder<Scheduler> sch = newScheduler({});
		Schedule *trig = nullptr;
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &inc>(&cnt1);
			c.name = "steady";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::External;
			c.action.bind<uint32*, &inc>(&cnt2);
			c.name = "external";
			trig = sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Schedule*, &trigger>(trig);
			c.name = "trigger";
			c.delay = 100000;
			c.priority = 50;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler*, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(cnt2 == 1);
		CAGE_TEST(trig->statsRunCount() == 1);
	}

	{
		CAGE_TESTCASE("steady schedule with statistics");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0;
		Schedule *trig = nullptr;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32*, &incRandomSleep>(&cnt1);
			c.name = "steady";
			c.period = 30000;
			trig = sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler*, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 100000 && duration < 300000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(trig->statsRunCount() == cnt1);
		CAGE_TEST(trig->statsDurationSum() > 15000 * (uint64)cnt1);
		CAGE_TEST(trig->statsDurationSum() < 25000 * (uint64)cnt1 + 100000);
		CAGE_TEST(trig->statsDurationMax() > 15000);
		CAGE_TEST(trig->statsDurationMax() < 50000);
		CAGE_TEST(trig->statsDelayMax() > 0);
		CAGE_TEST(trig->statsDelayMax() < 50000);
	}

	{
		CAGE_TESTCASE("basic lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->setLockstep(true);
		uint32 cnt1 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &inc>(&cnt1);
			c.name = "steady";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler *, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration < 10000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
	}

	{
		CAGE_TESTCASE("two schedules lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->setLockstep(true);
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &inc>(&cnt1);
			c.name = "longer";
			c.period = 50000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &inc>(&cnt2);
			c.name = "shorter";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler *, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration < 10000);
		}
		CAGE_TEST(cnt1 >= 2 && cnt1 <= 6);
		CAGE_TEST(cnt2 >= 4 && cnt2 <= 8);
	}

	{
		CAGE_TESTCASE("sleeping schedule lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->setLockstep(true);
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &inc>(&cnt1);
			c.name = "quick";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt2);
			c.name = "sleepy";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler *, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration > 4 * 30000 && duration < 8 * 30000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(cnt2 >= 4 && cnt2 <= 8);
	}

	{
		CAGE_TESTCASE("trigger external schedule lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->setLockstep(true);
		Schedule *trig = nullptr;
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &inc>(&cnt1);
			c.name = "steady";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::External;
			c.action.bind<uint32 *, &inc>(&cnt2);
			c.name = "external";
			trig = sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Schedule *, &trigger>(trig);
			c.name = "trigger";
			c.delay = 100000;
			c.priority = 50;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler *, &stop>(+sch);
			c.name = "terminator";
			c.delay = 200000;
			c.priority = 100;
			sch->newSchedule(c);
		}
		{
			Holder<Timer> tmr = newTimer();
			sch->run();
			const uint64 duration = tmr->microsSinceStart();
			CAGE_TEST(duration < 10000);
		}
		CAGE_TEST(cnt1 >= 4 && cnt1 <= 8);
		CAGE_TEST(cnt2 == 1);
		CAGE_TEST(trig->statsRunCount() == 1);
	}
}
