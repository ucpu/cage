#ifndef guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd
#define guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd

namespace cage
{
	enum class scheduleTypeEnum : uint32
	{
		Once, // this task is run once and is automatically deallocated afterwards
		SteadyPeriodic, // uses steady clock, maintaining the period from the beginning
		FreePeriodic, // its clock is reset every time the task is run
		External, // scheduling the task is triggered by external event
		Empty, // this task is run when no other tasks are scheduled
	};

	struct CAGE_API scheduleCreateConfig
	{
		scheduleCreateConfig();
		detail::stringBase<60> name;
		delegate<void()> action;
		uint64 delay; // used for all types of schedules
		uint64 period; // used for periodic schedules only
		scheduleTypeEnum type;
		sint32 priority; // higher priority is run earlier or more often
		uint32 maxSteadyPeriods; // when the schedule is not managing by this many runs, reset its timer (valid for steady periodic schedules only)
	};

	class CAGE_API schedule : private immovable
	{
	public:
		void trigger(); // valid for external schedule only; can be called from any thread (but beware of deallocation)
		void run(); // call the action
		void destroy(); // removes the schedule from the scheduler (and deallocates it)

		void period(uint64 p); // valid for periodic schedules only
		uint64 period() const;

		void priority(sint32 p); // set current priority
		sint32 priority() const;

		// statistics are not available for schemes of once type
		uint64 delayWindowAvg() const;
		uint64 delayWindowMax() const;
		uint64 delayTotalAvg() const;
		uint64 delayTotalMax() const;
		uint64 delayTotalSum() const;
		uint64 durationWindowAvg() const;
		uint64 durationWindowMax() const;
		uint64 durationTotalAvg() const;
		uint64 durationTotalMax() const;
		uint64 durationTotalSum() const;
		uint32 runsCount() const;
	};

	struct CAGE_API schedulerCreateConfig
	{
		schedulerCreateConfig();
		uint64 maxSleepDuration;
	};

	class CAGE_API scheduler : private immovable
	{
	public:
		void run();
		void stop(); // can be called from any thread

		schedule *newSchedule(const scheduleCreateConfig &config);
		void clear(); // removes all schedules from the scheduler (and deallocates them)

		sint32 latestPriority() const;
	};

	CAGE_API holder<scheduler> newScheduler(const schedulerCreateConfig &config);
}

#endif // guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd
