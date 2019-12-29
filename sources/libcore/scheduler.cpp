#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/scheduler.h>
#include <cage-core/timer.h>
#include <cage-core/math.h> // max
#include <cage-core/concurrent.h> // threadSleep
#include <cage-core/variableSmoothingBuffer.h>

#include <optick.h>

#include <vector>
#include <algorithm>
#include <atomic>

namespace cage
{
	namespace
	{
		struct schedStats : private Immovable
		{
			VariableSmoothingBuffer<uint64, 100> delays;
			VariableSmoothingBuffer<uint64, 100> durations;
			uint64 totalDelay;
			uint64 totalDuration;
			uint64 maxDelay;
			uint64 maxDuration;
			uint32 runs;

			schedStats() : totalDelay(0), totalDuration(0), maxDelay(0), maxDuration(0), runs(0)
			{}

			void add(uint64 delay, uint64 duration)
			{
				delays.add(delay);
				durations.add(duration);
				totalDelay += delay;
				totalDuration += duration;
				maxDelay = max(maxDelay, delay);
				maxDuration = max(maxDuration, duration);
				runs++;
			}
		};

		class schedulerImpl;

		class scheduleImpl : public Schedule
		{
		public:
			const ScheduleCreateConfig conf;
			schedulerImpl *const schr;
			Holder<schedStats> stats;
			uint64 sched;
			sint32 pri;
			std::atomic<bool> active;

			scheduleImpl(schedulerImpl *schr, const ScheduleCreateConfig &config) : conf(config), schr(schr), sched(m), pri(0), active(false)
			{
				if (conf.type != ScheduleTypeEnum::Once)
					stats = detail::systemArena().createHolder<schedStats>();
			}
		};

		class schedulerImpl : public Scheduler
		{
		public:
			const SchedulerCreateConfig conf;
			std::vector<Holder<scheduleImpl>> scheds;
			std::vector<scheduleImpl*> tmp;
			Holder<Timer> tmr;
			uint64 t;
			sint32 lastPriority;
			std::atomic<bool> stopping;

			schedulerImpl(const SchedulerCreateConfig &config) : conf(config), t(0), lastPriority(0), stopping(false)
			{
				tmr = newTimer();
			}

			void reset()
			{
				tmr->reset();
				for (const auto &it : scheds)
				{
					it->sched = m;
					it->active = it->conf.type == ScheduleTypeEnum::Empty;
				}
			}

			void checkNewSchedules()
			{
				for (const auto &it : scheds)
				{
					if (it->sched == m)
					{
						it->sched = t + it->conf.delay;
						it->pri = it->conf.priority;
					}
				}
			}

			void activateAllEmpty()
			{
				for (const auto &it : scheds)
				{
					if (it->conf.type == ScheduleTypeEnum::Empty)
						it->active = true;
				}
			}

			void filterAvailableSchedules()
			{
				for (const auto &it : scheds)
				{
					if (it->conf.type == ScheduleTypeEnum::Empty)
						continue;
					if (it->sched > t)
						continue;
					if (it->conf.type == ScheduleTypeEnum::External && !it->active)
						continue;
					tmp.push_back(it.get());
				}
				if (!tmp.empty())
				{
					activateAllEmpty();
					return;
				}
				for (const auto &it : scheds)
				{
					if (it->conf.type != ScheduleTypeEnum::Empty)
						continue;
					if (it->sched > t)
						continue;
					if (!it->active)
						continue;
					tmp.push_back(it.get());
				}
			}

			uint64 closestScheduleTime()
			{
				uint64 res = m;
				for (const auto &it : scheds)
				{
					if (it->conf.type == ScheduleTypeEnum::Empty)
						continue;
					if (it->conf.type == ScheduleTypeEnum::External && !it->active)
						continue;
					res = min(res, it->sched);
				}
				return res;
			}

			void goSleep()
			{
				OPTICK_EVENT();
				activateAllEmpty();
				uint64 s = closestScheduleTime() - t;
				s = min(s, conf.maxSleepDuration);
				s = max(s, (uint64)1000); // some systems do not have higher precision sleeps; this will prevent busy looping
				//CAGE_LOG(SeverityEnum::Info, "Scheduler", stringizer() + "scheduler is going to sleep for " + s + " us");
				threadSleep(s);
			}

			void sortSchedulesByPriority()
			{
				for (const auto &it : tmp)
					it->pri++;
				std::stable_sort(tmp.begin(), tmp.end(), [](const scheduleImpl *a, const scheduleImpl *b) {
					return a->pri > b->pri; // higher priority goes first
				});
			}

			void runSchedule()
			{
				scheduleImpl *s = tmp[0];
				//CAGE_LOG(SeverityEnum::Info, "Scheduler", stringizer() + "running schedule: " + s->conf.name);
				lastPriority = s->pri;
				s->pri = s->conf.priority;
				s->active = false;
				uint64 start = tmr->microsSinceStart();
				s->run(); // likely to throw
				uint64 end = tmr->microsSinceStart();
				if (s->stats)
					s->stats->add(start - s->sched, end - start);
				switch (s->conf.type)
				{
				case ScheduleTypeEnum::Once:
					return s->destroy();
				case ScheduleTypeEnum::SteadyPeriodic:
				{
					uint64 skip = (end - s->sched) / s->conf.period;
					if (skip >= s->conf.maxSteadyPeriods)
					{
						CAGE_LOG(SeverityEnum::Warning, "Scheduler", stringizer() + "schedule '" + s->conf.name + "' cannot keep up and will skip " + skip + " iterations");
						s->sched += skip * s->conf.period;
					}
					else
						s->sched += s->conf.period;
				} break;
				case ScheduleTypeEnum::FreePeriodic:
					s->sched = end + s->conf.period;
					break;
				}
			}

			void runIteration()
			{
				CAGE_ASSERT(!scheds.empty());
				tmp.clear();
				tmp.reserve(scheds.size());
				t = tmr->microsSinceStart();
				checkNewSchedules();
				filterAvailableSchedules();
				if (tmp.empty())
					return goSleep();
				sortSchedulesByPriority();
				runSchedule();
			}

			void run()
			{
				reset();
				stopping = false;
				while (!stopping && !scheds.empty())
					runIteration();
			}
		};
	}

	ScheduleCreateConfig::ScheduleCreateConfig() : name("<unnamed>"), delay(0), period(1000), type(ScheduleTypeEnum::Once), priority(0), maxSteadyPeriods(3)
	{}

	void Schedule::trigger()
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		CAGE_ASSERT(impl->conf.type == ScheduleTypeEnum::External);
		CAGE_ASSERT(impl->sched != m, "cannot trigger schedule before it was initialized");
		if (!impl->active)
		{
			// sched is updated to ensure meaningful delay statistics
			impl->sched = max(impl->sched, impl->schr->tmr->microsSinceStart()); // the schedule cannot be run before its initial delay is expired
			// potential race condition here; i think it does not hurt
			impl->active = true; // the atomic is updated last to commit memory barrier
		}
	}

	void Schedule::run()
	{
		OPTICK_EVENT();
		scheduleImpl *impl = (scheduleImpl*)this;
		OPTICK_TAG("schedule", impl->conf.name.c_str());
		if (!impl->conf.action)
			return;
		try
		{
			impl->conf.action();
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "exception in schedule '" + impl->conf.name + "'");
			throw;
		}
	}

	void Schedule::destroy()
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		auto &vec = impl->schr->scheds;
		auto it = std::find_if(vec.begin(), vec.end(), [&](const auto &a) { return a.get() == impl; });
		CAGE_ASSERT(it != vec.end());
		vec.erase(it);
	}

	void Schedule::period(uint64 p)
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		CAGE_ASSERT(impl->conf.type == ScheduleTypeEnum::SteadyPeriodic || impl->conf.type == ScheduleTypeEnum::FreePeriodic);
		const_cast<ScheduleCreateConfig&>(impl->conf).period = p;
	}

	uint64 Schedule::period() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		CAGE_ASSERT(impl->conf.type == ScheduleTypeEnum::SteadyPeriodic || impl->conf.type == ScheduleTypeEnum::FreePeriodic);
		return impl->conf.period;
	}

	void Schedule::priority(sint32 p)
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		impl->pri = p;
	}

	sint32 Schedule::priority() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->pri;
	}

	uint64 Schedule::delayWindowAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->delays.smooth();
	}

	uint64 Schedule::delayWindowMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->delays.max();
	}

	uint64 Schedule::delayTotalAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		if (impl->stats->runs)
			return impl->stats->totalDelay / impl->stats->runs;
		return 0;
	}

	uint64 Schedule::delayTotalMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->maxDelay;
	}

	uint64 Schedule::delayTotalSum() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->totalDelay;
	}

	uint64 Schedule::durationWindowAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->durations.smooth();
	}

	uint64 Schedule::durationWindowMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->durations.max();
	}

	uint64 Schedule::durationTotalAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		if (impl->stats->runs)
			return impl->stats->totalDuration / impl->stats->runs;
		return 0;
	}

	uint64 Schedule::durationTotalMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->maxDuration;
	}

	uint64 Schedule::durationTotalSum() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->totalDuration;
	}

	uint32 Schedule::runsCount() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->runs;
	}

	SchedulerCreateConfig::SchedulerCreateConfig() : maxSleepDuration(1000000)
	{}

	void Scheduler::run()
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		impl->run();
	}

	void Scheduler::stop()
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		impl->stopping = true;
	}

	Schedule *Scheduler::newSchedule(const ScheduleCreateConfig &config)
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		auto sch = detail::systemArena().createHolder<scheduleImpl>(impl, config);
		auto res = sch.get();
		impl->scheds.push_back(templates::move(sch));
		return res;
	}

	void Scheduler::clear()
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		impl->scheds.clear();
	}

	sint32 Scheduler::latestPriority() const
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		return impl->lastPriority;
	}

	Holder<Scheduler> newScheduler(const SchedulerCreateConfig &config)
	{
		return detail::systemArena().createImpl<Scheduler, schedulerImpl>(config);
	}
}
