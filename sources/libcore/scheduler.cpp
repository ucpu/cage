#include <cage-core/scheduler.h>
#include <cage-core/timer.h>
#include <cage-core/math.h> // max
#include <cage-core/concurrent.h> // threadSleep
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-core/profiling.h>

#include <vector>
#include <algorithm>
#include <atomic>

namespace cage
{
	void ScheduleStatistics::add(uint64 delay, uint64 duration)
	{
		delays.add(delay);
		durations.add(duration);
		totalDelay += delay;
		totalDuration += duration;
		maxDelay = max(maxDelay, delay);
		maxDuration = max(maxDuration, duration);
		runs++;
	}

	namespace
	{
		class SchedulerImpl;

		class ScheduleImpl : public Schedule
		{
		public:
			const ScheduleCreateConfig conf;
			Holder<ScheduleStatistics> stats;
			SchedulerImpl *schr = nullptr;
			uint64 sched = m;
			sint32 pri = 0;
			std::atomic<bool> active = false;

			explicit ScheduleImpl(SchedulerImpl *schr, const ScheduleCreateConfig &config) : conf(config), schr(schr)
			{
				CAGE_ASSERT(schr);
				if (conf.type != ScheduleTypeEnum::Once)
					stats = systemMemory().createHolder<ScheduleStatistics>();
			}

			~ScheduleImpl()
			{
				detach();
			}
		};

		class SchedulerImpl : public Scheduler
		{
		public:
			const SchedulerCreateConfig conf;
			std::vector<Holder<ScheduleImpl>> scheds;
			std::vector<ScheduleImpl*> tmp;
			Holder<Timer> realTimer;
			uint64 realDrift = 0; // offset for the real timer, this happens when switching lockstep mode
			uint64 t = 0; // current time for scheduling events
			uint64 lastTime = 0; // time at which the last schedule was run
			sint32 lastPriority = 0;
			std::atomic<bool> stopping = false;
			bool lockstepApi = false;
			bool lockstepEffective = false;

			explicit SchedulerImpl(const SchedulerCreateConfig &config) : conf(config)
			{
				realTimer = newTimer();
			}

			void reset()
			{
				realTimer->reset();
				realDrift = 0;
				t = 0;
				lastTime = 0;
				lastPriority = 0;
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
					tmp.push_back(+it);
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
					tmp.push_back(+it);
				}
			}

			uint64 adjustedRealTime()
			{
				return realTimer->duration() + realDrift;
			}

			uint64 currentTime()
			{
				return lockstepEffective ? t : adjustedRealTime();
			}

			uint64 minimalScheduleTime()
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
				ProfilingScope profiling("sleep", "scheduler");
				activateAllEmpty();
				uint64 s = minimalScheduleTime() - t;
				s = min(s, conf.maxSleepDuration);
				s = max(s, (uint64)1000); // some systems do not have higher precision sleeps; this will prevent busy looping
				//CAGE_LOG(SeverityEnum::Info, "scheduler", stringizer() + "scheduler is going to sleep for " + s + " us");
				threadSleep(s);
			}

			void sortSchedulesByPriority()
			{
				for (const auto &it : tmp)
					it->pri++;
				std::stable_sort(tmp.begin(), tmp.end(), [](const ScheduleImpl *a, const ScheduleImpl *b) {
					return a->pri > b->pri; // higher priority goes first
				});
			}

			void runSchedule()
			{
				ScheduleImpl *s = tmp[0];
				//CAGE_LOG(SeverityEnum::Info, "scheduler", stringizer() + "running schedule: " + s->conf.name);
				lastTime = s->sched;
				lastPriority = s->pri;
				s->pri = s->conf.priority;
				s->active = false;
				const uint64 start = currentTime();
				s->run(); // likely to throw
				const uint64 end = currentTime();
				if (s->stats)
					s->stats->add(start - s->sched, end - start);
				switch (s->conf.type)
				{
				case ScheduleTypeEnum::Once:
					return s->detach();
				case ScheduleTypeEnum::SteadyPeriodic:
				{
					const uint64 skip = (end - s->sched) / s->conf.period;
					if (skip >= s->conf.maxSteadyPeriods)
					{
						CAGE_LOG(SeverityEnum::Warning, "scheduler", stringizer() + "schedule '" + s->conf.name + "' cannot keep up and will skip " + skip + " iterations");
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
				if (lockstepEffective != lockstepApi)
				{
					if (!lockstepApi)
					{
						realDrift = t;
						realTimer->reset();
					}
					lockstepEffective = lockstepApi;
				}
				t = lockstepEffective ? minimalScheduleTime() : adjustedRealTime();
				//CAGE_LOG_DEBUG(SeverityEnum::Info, "scheduler", stringizer() + "current time: " + t);
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
				checkNewSchedules();
				stopping = false;
				while (!stopping && !scheds.empty())
					runIteration();
			}
		};
	}

	void Schedule::trigger()
	{
		ScheduleImpl *impl = (ScheduleImpl *)this;
		CAGE_ASSERT(impl->conf.type == ScheduleTypeEnum::External);
		CAGE_ASSERT(impl->sched != m);
		if (!impl->active)
		{
			// sched is updated to ensure meaningful delay statistics
			impl->sched = max(impl->sched, impl->schr->currentTime()); // the schedule cannot be run before its initial delay is expired
			// potential race condition here; i think it does not hurt
			impl->active = true; // the atomic is updated last to commit memory barrier
		}
	}

	void Schedule::run()
	{
		ScheduleImpl *impl = (ScheduleImpl *)this;
		ProfilingScope profiling(impl->conf.name, "scheduler");
		if (!impl->conf.action)
			return;
		try
		{
			impl->conf.action();
		}
		catch (...)
		{
			CAGE_LOG_THROW(stringizer() + "exception in schedule '" + impl->conf.name + "'");
			throw;
		}
	}

	void Schedule::detach()
	{
		ScheduleImpl *impl = (ScheduleImpl *)this;
		if (!impl->schr)
			return; // already detached
		auto &vec = impl->schr->scheds;
		impl->schr = nullptr;
		auto it = std::find_if(vec.begin(), vec.end(), [&](const auto &a) { return +a == impl; });
		if (it != vec.end())
			vec.erase(it);
	}

	void Schedule::period(uint64 p)
	{
		ScheduleImpl *impl = (ScheduleImpl *)this;
		CAGE_ASSERT(impl->conf.type == ScheduleTypeEnum::SteadyPeriodic || impl->conf.type == ScheduleTypeEnum::FreePeriodic);
		const_cast<ScheduleCreateConfig &>(impl->conf).period = p;
	}

	uint64 Schedule::period() const
	{
		const ScheduleImpl *impl = (const ScheduleImpl *)this;
		CAGE_ASSERT(impl->conf.type == ScheduleTypeEnum::SteadyPeriodic || impl->conf.type == ScheduleTypeEnum::FreePeriodic);
		return impl->conf.period;
	}

	void Schedule::priority(sint32 p)
	{
		ScheduleImpl *impl = (ScheduleImpl *)this;
		impl->pri = p;
	}

	sint32 Schedule::priority() const
	{
		const ScheduleImpl *impl = (const ScheduleImpl *)this;
		return impl->pri;
	}

	uint64 Schedule::time() const
	{
		const ScheduleImpl *impl = (const ScheduleImpl *)this;
		return impl->sched;
	}

	const ScheduleStatistics &Schedule::statistics() const
	{
		const ScheduleImpl *impl = (const ScheduleImpl *)this;
		return *impl->stats;
	}

	void Scheduler::run()
	{
		SchedulerImpl *impl = (SchedulerImpl *)this;
		impl->run();
	}

	void Scheduler::stop()
	{
		SchedulerImpl *impl = (SchedulerImpl *)this;
		impl->stopping = true;
	}

	Holder<Schedule> Scheduler::newSchedule(const ScheduleCreateConfig &config)
	{
		SchedulerImpl *impl = (SchedulerImpl *)this;
		auto sch = systemMemory().createHolder<ScheduleImpl>(impl, config);
		impl->scheds.push_back(sch.share());
		return std::move(sch).cast<Schedule>();
	}

	void Scheduler::clear()
	{
		SchedulerImpl *impl = (SchedulerImpl *)this;
		impl->scheds.clear();
	}

	void Scheduler::lockstep(bool enable)
	{
		SchedulerImpl *impl = (SchedulerImpl *)this;
		if (impl->lockstepApi == enable)
			return;
		CAGE_LOG(SeverityEnum::Warning, "scheduler", stringizer() + "scheduler lockstep mode: " + enable);
		impl->lockstepApi = enable;
	}

	bool Scheduler::lockstep() const
	{
		const SchedulerImpl *impl = (const SchedulerImpl *)this;
		return impl->lockstepApi;
	}

	uint64 Scheduler::latestTime() const
	{
		const SchedulerImpl *impl = (const SchedulerImpl *)this;
		return impl->lastTime;
	}

	sint32 Scheduler::latestPriority() const
	{
		const SchedulerImpl *impl = (const SchedulerImpl *)this;
		return impl->lastPriority;
	}

	Holder<Scheduler> newScheduler(const SchedulerCreateConfig &config)
	{
		return systemMemory().createImpl<Scheduler, SchedulerImpl>(config);
	}
}
