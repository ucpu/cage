#include <algorithm>
#include <array>
#include <atomic>
#include <vector>

#include "main.h"

#include <cage-core/string.h>
#include <cage-core/timer.h>

namespace
{
	template<class T>
	void doNotOptimizeAway(const T &value)
	{
#if defined(__GNUC__) || defined(__clang__)
		asm volatile("" : : "r,m"(value) : "memory");
#elif defined(_MSC_VER)
		_ReadWriteBarrier();
		(void)value;
#else
		std::atomic_signal_fence(std::memory_order_seq_cst);
		(void)value;
#endif
	}

	void testMemset()
	{
		CAGE_TESTCASE("memset");

		std::vector<uint32> vec;
		vec.resize(CAGE_DEBUG_BOOL ? 10'000 : 1'000'000);

		Holder<Timer> timer = newTimer();
		std::array<uint64, 3> durations = {};
		for (uint32 iteration = 0; iteration < 3; iteration++)
		{
			std::atomic_signal_fence(std::memory_order_seq_cst);
			timer->reset();
			detail::memset(vec.data(), 0, sizeof(uint32) * vec.size());
			doNotOptimizeAway(vec[iteration % vec.size()]);
			durations[iteration] = timer->elapsed();
		}

		std::sort(durations.begin(), durations.end());
		CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "memset duration: " + durations[1] + " us");
	}

	void testCustom()
	{
		CAGE_TESTCASE("custom");

		std::vector<uint32> vec;
		vec.resize(CAGE_DEBUG_BOOL ? 10'000 : 1'000'000);

		Holder<Timer> timer = newTimer();
		std::array<uint64, 3> durations = {};
		for (uint32 iteration = 0; iteration < 3; iteration++)
		{
			std::atomic_signal_fence(std::memory_order_seq_cst);
			timer->reset();
			for (uint32 &i : vec)
				i = 0;
			doNotOptimizeAway(vec[iteration % vec.size()]);
			durations[iteration] = timer->elapsed();
		}

		std::sort(durations.begin(), durations.end());
		CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "custom duration: " + durations[1] + " us");
	}
}

void testSimdPerformance()
{
	CAGE_TESTCASE("simd performance");
	testMemset();
	testCustom();
}
