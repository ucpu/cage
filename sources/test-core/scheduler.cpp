#include "main.h"

#include <cage-core/concurrent.h> // threadSleep
#include <cage-core/math.h> // randomRange
#include <cage-core/scheduler.h>

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

	uint64 lastCheckedTime = 0;

	void monotonicTime(Scheduler *s)
	{
		const uint64 t = s->latestTime();
		CAGE_TEST(t >= lastCheckedTime);
		lastCheckedTime = t;
	}

	void enableLockstep(Scheduler *s)
	{
		s->lockstep(true);
	}

	void disableLockstep(Scheduler *s)
	{
		s->lockstep(false);
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
			c.action.bind<uint32 *, &inc>(&periodicCount);
			c.name = "periodic add one";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Empty;
			c.action.bind<uint32 *, &inc>(&emptyCount);
			c.name = "empty add one";
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
		CAGE_TEST(periodicCount == 0);
		CAGE_TEST(emptyCount == 0);
		sch->run();
		CAGE_TEST(periodicCount >= 1 && periodicCount <= 30);
		CAGE_TEST(emptyCount >= 1 && emptyCount <= 30);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("two steady schedules with different periods");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt1);
			c.name = "frequent";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt2);
			c.name = "infrequent";
			c.period = 80000;
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(cnt2 >= 1 && cnt2 <= 30);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("two steady schedules with different priorities");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt1);
			c.name = "low priority";
			c.period = 30000;
			c.priority = 1;
			c.maxSteadyPeriods = m; // do not skip, we need to test that the higher priority is run more often
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt2);
			c.name = "high priority";
			c.period = 30000;
			c.priority = 3;
			c.maxSteadyPeriods = m;
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
		sch->run();
		const uint32 sum = cnt1 + cnt2;
		CAGE_TEST(sum >= 1 && sum <= 30);
		CAGE_TEST(cnt2 > cnt1);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("steady and free periodic schedules");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0, cnt2 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt1);
			c.name = "steady";
			c.period = 50000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::FreePeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt2);
			c.name = "free";
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(cnt2 >= 1 && cnt2 <= 30);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("trigger external schedule");
		Holder<Scheduler> sch = newScheduler({});
		Holder<Schedule> trig;
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
			c.action.bind<Schedule *, &trigger>(+trig);
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(cnt2 == 1);
		CAGE_TEST(trig->statistics().runs == 1);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("steady schedule with statistics");
		Holder<Scheduler> sch = newScheduler({});
		uint32 cnt1 = 0;
		Holder<Schedule> trig;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt1);
			c.name = "steady";
			c.period = 30000;
			trig = sch->newSchedule(c);
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(trig->statistics().runs == cnt1);
		CAGE_TEST(trig->statistics().totalDuration > 15'000 * (uint64)cnt1);
		CAGE_TEST(trig->statistics().totalDuration < 50'000 * (uint64)cnt1 + 300'000);
		CAGE_TEST(trig->statistics().maxDuration > 15'000);
		CAGE_TEST(trig->statistics().maxDuration < 200'000);
		CAGE_TEST(trig->statistics().maxDelay > 0);
		CAGE_TEST(trig->statistics().maxDelay < 200'000);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("basic lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->lockstep(true);
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("two schedules lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->lockstep(true);
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(cnt2 >= 1 && cnt2 <= 30);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("sleeping schedule lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->lockstep(true);
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(cnt2 >= 1 && cnt2 <= 30);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("slower schedule lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->lockstep(true);
		uint32 cnt1 = 0;
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<uint32 *, &incRandomSleep>(&cnt1);
			c.name = "sleepy";
			c.period = 10000;
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
		sch->run();
		CAGE_TEST(cnt1 >= 10 && cnt1 <= 50);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("trigger external schedule lockstep scheduler");
		Holder<Scheduler> sch = newScheduler({});
		sch->lockstep(true);
		Holder<Schedule> trig;
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
			c.action.bind<Schedule *, &trigger>(+trig);
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
		sch->run();
		CAGE_TEST(cnt1 >= 1 && cnt1 <= 30);
		CAGE_TEST(cnt2 == 1);
		CAGE_TEST(trig->statistics().runs == 1);
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("lockstep scheduler monotonic time");
		Holder<Scheduler> sch = newScheduler({});
		lastCheckedTime = 0;
		sch->lockstep(true);
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<Scheduler *, &monotonicTime>(+sch);
			c.name = "monotonic";
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
		sch->run();
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("enabling lockstep midway");
		Holder<Scheduler> sch = newScheduler({});
		lastCheckedTime = 0;
		sch->lockstep(false);
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<Scheduler *, &monotonicTime>(+sch);
			c.name = "monotonic";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler *, &enableLockstep>(+sch);
			c.name = "enabler";
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
		sch->run();
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("disabling lockstep midway");
		Holder<Scheduler> sch = newScheduler({});
		lastCheckedTime = 0;
		sch->lockstep(true);
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<Scheduler *, &monotonicTime>(+sch);
			c.name = "monotonic";
			c.period = 30000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::Once;
			c.action.bind<Scheduler *, &disableLockstep>(+sch);
			c.name = "disabler";
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
		sch->run();
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("repeatedly switching lockstep");
		Holder<Scheduler> sch = newScheduler({});
		lastCheckedTime = 0;
		sch->lockstep(false);
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<Scheduler *, &monotonicTime>(+sch);
			c.name = "monotonic";
			c.period = 5000;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<Scheduler *, &enableLockstep>(+sch);
			c.name = "enabler";
			c.delay = 0;
			c.period = 30000;
			c.priority = 50;
			sch->newSchedule(c);
		}
		{
			ScheduleCreateConfig c;
			c.type = ScheduleTypeEnum::SteadyPeriodic;
			c.action.bind<Scheduler *, &disableLockstep>(+sch);
			c.name = "disabler";
			c.delay = 30000;
			c.period = 30000;
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
		sch->run();
		CAGE_LOG(SeverityEnum::Info, "scheduler", Stringizer() + "utilization: " + (sch->utilization() * 100) + " %");
	}

	{
		CAGE_TESTCASE("repeatedly detached schedule");
		{
			Holder<Scheduler> sch = newScheduler({});
			Holder<Schedule> a = sch->newSchedule({});
			a->detach();
			a->detach();
			sch->clear();
		}
		{
			Holder<Scheduler> sch = newScheduler({});
			Holder<Schedule> a = sch->newSchedule({});
			sch->clear();
			a->detach();
			a->detach();
		}
	}
}
