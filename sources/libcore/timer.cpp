#ifdef CAGE_SYSTEM_WINDOWS
	#include "windowsMinimumInclude.h"
#else
	#include <time.h>
#endif

#ifndef CAGE_SYSTEM_WINDOWS
static constexpr clockid_t clockid = CLOCK_REALTIME;
#endif

#include <cage-core/timer.h>

namespace cage
{
	namespace
	{
		class TimerImpl : public Timer
		{
		public:
			Time last = {};

#ifdef CAGE_SYSTEM_WINDOWS
			LARGE_INTEGER begin = {}; // in counts
#else
			struct timespec begin = {};
#endif

			TimerImpl() { reset(); }
		};

#ifdef CAGE_SYSTEM_WINDOWS

		LARGE_INTEGER frequencyInitializer()
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			return freq;
		}

		LARGE_INTEGER frequency()
		{
			static const LARGE_INTEGER freq = frequencyInitializer();
			return freq;
		}

#else

		Time convert(const struct timespec &t)
		{
			return Time(t.tv_sec * (uint64)1'000'000 + t.tv_nsec / (uint64)1'000);
		}

#endif
	}

	void Timer::reset()
	{
		TimerImpl *impl = (TimerImpl *)this;
#ifdef CAGE_SYSTEM_WINDOWS
		QueryPerformanceCounter(&impl->begin);
#else
		clock_gettime(clockid, &impl->begin);
#endif
		impl->last = {};
	}

	Time Timer::duration()
	{
		TimerImpl *impl = (TimerImpl *)this;

#ifdef CAGE_SYSTEM_WINDOWS

		LARGE_INTEGER curr;
		QueryPerformanceCounter(&curr);
		return Time((uint64)1'000'000 * (curr.QuadPart - impl->begin.QuadPart) / frequency().QuadPart);

#else

		struct timespec curr;
		clock_gettime(clockid, &curr);
		return convert(curr) - convert(impl->begin);

#endif
	}

	Time Timer::elapsed()
	{
		TimerImpl *impl = (TimerImpl *)this;
		Time curr = duration();
		Time res = curr - impl->last;
		impl->last = curr;
		return res;
	}

	Holder<Timer> newTimer()
	{
		return systemMemory().createImpl<Timer, TimerImpl>();
	}
}
