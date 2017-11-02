#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/utility/timer.h>
#include "../system.h"

#ifndef CAGE_SYSTEM_WINDOWS
static const clockid_t clockid = CLOCK_REALTIME;
#endif

namespace cage
{
	namespace
	{
		class timerImpl : public timerClass
		{
		public:
			uint64 last; // in micros

#ifdef CAGE_SYSTEM_WINDOWS
			LARGE_INTEGER begin; // in counts
#else
			struct timespec begin;
#endif

			timerImpl()
			{
				reset();
			}
		};

#ifdef CAGE_SYSTEM_WINDOWS

		const LARGE_INTEGER frequencyInitializer()
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			return freq;
		}

		const LARGE_INTEGER frequency()
		{
			static LARGE_INTEGER freq = frequencyInitializer();
			return freq;
		}

#else

		const uint64 convert(const struct timespec &t)
		{
			return t.tv_sec * 1000000 + t.tv_nsec / 1000;
		}

#endif
	}

	void timerClass::reset()
	{
		timerImpl *impl = (timerImpl*)this;
#ifdef CAGE_SYSTEM_WINDOWS
		QueryPerformanceCounter(&impl->begin);
#else
		clock_gettime(clockid, &impl->begin);
#endif
		impl->last = 0;
	}

	uint64 timerClass::microsSinceStart()
	{
		timerImpl *impl = (timerImpl*)this;

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

	uint64 timerClass::microsSinceLast()
	{
		timerImpl *impl = (timerImpl*)this;
		uint64 curr = microsSinceStart();
		uint64 res = curr - impl->last;
		impl->last = curr;
		return res;
	}

	holder<timerClass> newTimer()
	{
		return detail::systemArena().createImpl<timerClass, timerImpl>();
	}

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
		inline string fill(uint32 a)
		{
			if (a < 10)
				return string() + "0" + a;
			return a;
		}
	}

	string formatDateTime(uint32 y, uint32 M, uint32 d, uint32 h, uint32 m, uint32 s)
	{
		return string() + y + "-" + fill(M) + "-" + fill(d) + " " + fill(h) + ":" + fill(m) + ":" + fill(s);
	}
}
