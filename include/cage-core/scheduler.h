#ifndef guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd
#define guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd

namespace cage
{
	enum class ScheduleTypeEnum : uint32
	{
		Once, // this task is run once and is automatically deallocated afterwards
		SteadyPeriodic, // uses steady clock, maintaining the period from the beginning
		FreePeriodic, // its clock is reset every time the task is run
		External, // scheduling the task is triggered by external event
		Empty, // this task is run when no other tasks are scheduled
	};

	struct CAGE_API ScheduleCreateConfig
	{
		detail::StringBase<64> name = "<unnamed>";
		Delegate<void()> action;
		uint64 delay = 0; // used for all types of schedules
		uint64 period = 1000; // used for periodic schedules only
		ScheduleTypeEnum type = ScheduleTypeEnum::Once;
		sint32 priority = 0; // higher priority is run earlier or more often
		uint32 maxSteadyPeriods = 3; // when the schedule is not managing by this many runs, reset its timer (valid for steady periodic schedules only)
	};

	class CAGE_API Schedule : private Immovable
	{
	public:
		void trigger(); // valid for external schedule only; can be called from any thread (but beware of deallocation)
		void run(); // call the action
		void destroy(); // removes the schedule from the scheduler (and deallocates it)

		void period(uint64 p); // valid for periodic schedules only
		uint64 period() const;

		void priority(sint32 p); // set current priority
		sint32 priority() const;

		uint64 time() const;

		// statistics are not available for schemes of once type
		static const uint32 StatisticsWindowSize = 100;
		const VariableSmoothingBuffer<uint64, StatisticsWindowSize> &statsDelay() const;
		const VariableSmoothingBuffer<uint64, StatisticsWindowSize> &statsDuration() const;
		uint64 statsDelayMax() const;
		uint64 statsDelaySum() const;
		uint64 statsDurationMax() const;
		uint64 statsDurationSum() const;
		uint32 statsRunCount() const;
	};

	struct CAGE_API SchedulerCreateConfig
	{
		uint64 maxSleepDuration = 1000000;
	};

	class CAGE_API Scheduler : private Immovable
	{
	public:
		void run();
		void stop(); // can be called from any thread

		Schedule *newSchedule(const ScheduleCreateConfig &config);
		void clear(); // removes all schedules from the scheduler (and deallocates them)

		sint32 latestPriority() const;
	};

	CAGE_API Holder<Scheduler> newScheduler(const SchedulerCreateConfig &config);
}

#endif // guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd
