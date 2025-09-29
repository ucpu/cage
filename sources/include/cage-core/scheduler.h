#ifndef guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd
#define guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd

#include <cage-core/core.h>

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

	struct CAGE_CORE_API ScheduleCreateConfig
	{
		StringPointer name = "<unnamed>";
		Delegate<void()> action;
		uint64 delay = 0; // used for all types of schedules
		uint64 period = 1000; // used for periodic schedules only
		ScheduleTypeEnum type = ScheduleTypeEnum::Once;
		sint32 priority = 0; // higher priority is run earlier or more often
		uint32 maxSteadyPeriods = 5; // when the schedule is not managing by this many runs, reset its timer (valid for steady periodic schedules only)
	};

	struct ScheduleStatistics : private Immovable
	{
		uint64 latestDelay = 0;
		uint64 latestDuration = 0;
		uint64 avgDelay = 0;
		uint64 avgDuration = 0;
		uint64 maxDelay = 0;
		uint64 maxDuration = 0;
		uint64 totalDelay = 0;
		uint64 totalDuration = 0;
		uint32 runs = 0;
	};

	class CAGE_CORE_API Schedule : private Immovable
	{
	public:
		void trigger(); // valid for external schedule only; can be called from any thread (but beware of deallocation)
		void run(); // call the action

		void detach(); // removes the schedule from the scheduler

		void period(uint64 p); // valid for periodic schedules only
		uint64 period() const;

		void priority(sint32 p); // set current priority
		sint32 priority() const;

		uint64 time() const;

		// statistics are not available for schedules of once type
		const ScheduleStatistics &statistics() const;
	};

	struct CAGE_CORE_API SchedulerCreateConfig
	{
		uint64 maxSleepDuration = 1'000'000;
	};

	class CAGE_CORE_API Scheduler : private Immovable
	{
	public:
		void run();
		void resume();
		void stop(); // can be called from any thread

		Holder<Schedule> newSchedule(const ScheduleCreateConfig &config);
		void clear(); // removes all schedules from the scheduler

		void lockstep(bool enable);
		bool lockstep() const;

		uint64 latestTime() const;
		sint32 latestPriority() const;

		float utilization() const;
	};

	CAGE_CORE_API Holder<Scheduler> newScheduler(const SchedulerCreateConfig &config);
}

#endif // guard_scheduler_h_dsg54e64hg56fr4jh125vtkj5fd
