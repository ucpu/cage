#include <cage-core/timer.h>

#ifdef CAGE_SYSTEM_WINDOWS
	#include "incWin.h"
#else
	#include <time.h>
#endif

#ifndef CAGE_SYSTEM_WINDOWS
static constexpr clockid_t clockid = CLOCK_REALTIME;
#endif

namespace cage
{
	namespace
	{
		class TimerImpl : public Timer
		{
		public:
			uint64 last = 0; // in micros

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

		uint64 convert(const struct timespec &t)
		{
			return t.tv_sec * 1000000 + t.tv_nsec / 1000;
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
		impl->last = 0;
	}

	uint64 Timer::duration()
	{
		TimerImpl *impl = (TimerImpl *)this;

#ifdef CAGE_SYSTEM_WINDOWS

		LARGE_INTEGER curr;
		QueryPerformanceCounter(&curr);
		return 1000000 * (curr.QuadPart - impl->begin.QuadPart) / frequency().QuadPart;

#else

		struct timespec curr;
		clock_gettime(clockid, &curr);
		return convert(curr) - convert(impl->begin);

#endif
	}

	uint64 Timer::elapsed()
	{
		TimerImpl *impl = (TimerImpl *)this;
		uint64 curr = duration();
		uint64 res = curr - impl->last;
		impl->last = curr;
		return res;
	}

	Holder<Timer> newTimer()
	{
		return systemMemory().createImpl<Timer, TimerImpl>();
	}
}
