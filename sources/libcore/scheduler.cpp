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
		struct schedStats : private immovable
		{
			variableSmoothingBuffer<uint64, 100> delays;
			variableSmoothingBuffer<uint64, 100> durations;
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

		class scheduleImpl : public schedule
		{
		public:
			const scheduleCreateConfig conf;
			schedulerImpl *const schr;
			holder<schedStats> stats;
			uint64 sched;
			sint32 pri;
			std::atomic<bool> active;

			scheduleImpl(schedulerImpl *schr, const scheduleCreateConfig &config) : conf(config), schr(schr), sched(m), pri(0), active(false)
			{
				if (conf.type != scheduleTypeEnum::Once)
					stats = detail::systemArena().createHolder<schedStats>();
			}
		};

		class schedulerImpl : public scheduler
		{
		public:
			const schedulerCreateConfig conf;
			std::vector<holder<scheduleImpl>> scheds;
			std::vector<scheduleImpl*> tmp;
			holder<timer> tmr;
			uint64 t;
			sint32 lastPriority;
			std::atomic<bool> stopping;

			schedulerImpl(const schedulerCreateConfig &config) : conf(config), t(0), lastPriority(0), stopping(false)
			{
				tmr = newTimer();
			}

			void reset()
			{
				tmr->reset();
				for (const auto &it : scheds)
				{
					it->sched = m;
					it->active = it->conf.type == scheduleTypeEnum::Empty;
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
					if (it->conf.type == scheduleTypeEnum::Empty)
						it->active = true;
				}
			}

			void filterAvailableSchedules()
			{
				for (const auto &it : scheds)
				{
					if (it->conf.type == scheduleTypeEnum::Empty)
						continue;
					if (it->sched > t)
						continue;
					if (it->conf.type == scheduleTypeEnum::External && !it->active)
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
					if (it->conf.type != scheduleTypeEnum::Empty)
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
					if (it->conf.type == scheduleTypeEnum::Empty)
						continue;
					if (it->conf.type == scheduleTypeEnum::External && !it->active)
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
				//CAGE_LOG(severityEnum::Info, "scheduler", stringizer() + "scheduler is going to sleep for " + s + " us");
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
				//CAGE_LOG(severityEnum::Info, "scheduler", stringizer() + "running schedule: " + s->conf.name);
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
				case scheduleTypeEnum::Once:
					return s->destroy();
				case scheduleTypeEnum::SteadyPeriodic:
				{
					uint64 skip = (end - s->sched) / s->conf.period;
					if (skip >= s->conf.maxSteadyPeriods)
					{
						CAGE_LOG(severityEnum::Warning, "scheduler", stringizer() + "schedule '" + s->conf.name + "' cannot keep up and will skip " + skip + " iterations");
						s->sched += skip * s->conf.period;
					}
					else
						s->sched += s->conf.period;
				} break;
				case scheduleTypeEnum::FreePeriodic:
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

	scheduleCreateConfig::scheduleCreateConfig() : name("<unnamed>"), delay(0), period(1000), type(scheduleTypeEnum::Once), priority(0), maxSteadyPeriods(3)
	{}

	void schedule::trigger()
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		CAGE_ASSERT(impl->conf.type == scheduleTypeEnum::External);
		CAGE_ASSERT(impl->sched != m, "cannot trigger schedule before it was initialized");
		if (!impl->active)
		{
			// sched is updated to ensure meaningful delay statistics
			impl->sched = max(impl->sched, impl->schr->tmr->microsSinceStart()); // the schedule cannot be run before its initial delay is expired
			// potential race condition here; i think it does not hurt
			impl->active = true; // the atomic is updated last to commit memory barrier
		}
	}

	void schedule::run()
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
			CAGE_LOG(severityEnum::Note, "exception", stringizer() + "exception in schedule '" + impl->conf.name + "'");
			throw;
		}
	}

	void schedule::destroy()
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		auto &vec = impl->schr->scheds;
		auto it = std::find_if(vec.begin(), vec.end(), [&](const auto &a) { return a.get() == impl; });
		CAGE_ASSERT(it != vec.end());
		vec.erase(it);
	}

	void schedule::period(uint64 p)
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		CAGE_ASSERT(impl->conf.type == scheduleTypeEnum::SteadyPeriodic || impl->conf.type == scheduleTypeEnum::FreePeriodic);
		const_cast<scheduleCreateConfig&>(impl->conf).period = p;
	}

	uint64 schedule::period() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		CAGE_ASSERT(impl->conf.type == scheduleTypeEnum::SteadyPeriodic || impl->conf.type == scheduleTypeEnum::FreePeriodic);
		return impl->conf.period;
	}

	void schedule::priority(sint32 p)
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		impl->pri = p;
	}

	sint32 schedule::priority() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->pri;
	}

	uint64 schedule::delayWindowAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->delays.smooth();
	}

	uint64 schedule::delayWindowMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->delays.max();
	}

	uint64 schedule::delayTotalAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		if (impl->stats->runs)
			return impl->stats->totalDelay / impl->stats->runs;
		return 0;
	}

	uint64 schedule::delayTotalMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->maxDelay;
	}

	uint64 schedule::delayTotalSum() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->totalDelay;
	}

	uint64 schedule::durationWindowAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->durations.smooth();
	}

	uint64 schedule::durationWindowMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->durations.max();
	}

	uint64 schedule::durationTotalAvg() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		if (impl->stats->runs)
			return impl->stats->totalDuration / impl->stats->runs;
		return 0;
	}

	uint64 schedule::durationTotalMax() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->maxDuration;
	}

	uint64 schedule::durationTotalSum() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->totalDuration;
	}

	uint32 schedule::runsCount() const
	{
		scheduleImpl *impl = (scheduleImpl*)this;
		return impl->stats->runs;
	}

	schedulerCreateConfig::schedulerCreateConfig() : maxSleepDuration(1000000)
	{}

	void scheduler::run()
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		impl->run();
	}

	void scheduler::stop()
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		impl->stopping = true;
	}

	schedule *scheduler::newSchedule(const scheduleCreateConfig &config)
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		auto sch = detail::systemArena().createHolder<scheduleImpl>(impl, config);
		auto res = sch.get();
		impl->scheds.push_back(templates::move(sch));
		return res;
	}

	void scheduler::clear()
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		impl->scheds.clear();
	}

	sint32 scheduler::latestPriority() const
	{
		schedulerImpl *impl = (schedulerImpl*)this;
		return impl->lastPriority;
	}

	holder<scheduler> newScheduler(const schedulerCreateConfig &config)
	{
		return detail::systemArena().createImpl<scheduler, schedulerImpl>(config);
	}
}
