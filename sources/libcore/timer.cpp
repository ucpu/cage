#include <cage-core/timer.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "incWin.h"
#else
#include <time.h>
#endif

#ifndef CAGE_SYSTEM_WINDOWS
static const clockid_t clockid = CLOCK_REALTIME;
#endif

namespace cage
{
	namespace
	{
		class TimerImpl : public Timer
		{
		public:
			uint64 last; // in micros

#ifdef CAGE_SYSTEM_WINDOWS
			LARGE_INTEGER begin; // in counts
#else
			struct timespec begin;
#endif

			TimerImpl()
			{
				reset();
			}
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
			static LARGE_INTEGER freq = frequencyInitializer();
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
		TimerImpl *impl = (TimerImpl*)this;
#ifdef CAGE_SYSTEM_WINDOWS
		QueryPerformanceCounter(&impl->begin);
#else
		clock_gettime(clockid, &impl->begin);
#endif
		impl->last = 0;
	}

	uint64 Timer::microsSinceStart()
	{
		TimerImpl *impl = (TimerImpl*)this;

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

	uint64 Timer::microsSinceLast()
	{
		TimerImpl *impl = (TimerImpl*)this;
		uint64 curr = microsSinceStart();
		uint64 res = curr - impl->last;
		impl->last = curr;
		return res;
	}

	Holder<Timer> newTimer()
	{
		return detail::systemArena().createImpl<Timer, TimerImpl>();
	}

	namespace detail
	{
		void getSystemDateTime(uint32 &year, uint32 &month, uint32 &day, uint32 &hour, uint32 &minute, uint32 &second)
		{
#ifdef CAGE_SYSTEM_WINDOWS

			SYSTEMTIME st;
			GetSystemTime(&st);
			year = st.wYear;
			month = st.wMonth;
			day = st.wDay;
			hour = st.wHour;
			minute = st.wMinute;
			second = st.wSecond;

#else

			time_t rawtime = time(nullptr);
			struct tm st;
			localtime_r(&rawtime, &st);
			year = st.tm_year + 1900;
			month = st.tm_mon + 1;
			day = st.tm_mday;
			hour = st.tm_hour;
			minute = st.tm_min;
			second = st.tm_sec;

#endif
		}

		namespace
		{
			string fill(uint32 a)
			{
				if (a < 10)
					return stringizer() + "0" + a;
				return string(a);
			}
		}

		string formatDateTime(uint32 y, uint32 M, uint32 d, uint32 h, uint32 m, uint32 s)
		{
			return stringizer() + y + "-" + fill(M) + "-" + fill(d) + " " + fill(h) + ":" + fill(m) + ":" + fill(s);
		}
	}
}
