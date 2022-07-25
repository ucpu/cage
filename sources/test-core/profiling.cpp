#include "main.h"
#include <cage-core/profiling.h>
#include <cage-core/config.h>

#include <atomic>

namespace
{
	void someMeaninglessWork()
	{
		std::atomic<int> v = 0;
		for (uint32 i = 0; i < 1000; i++)
			v += i;
	}
}

void testProfiling()
{
	CAGE_TESTCASE("profiling");

#ifdef CAGE_PROFILING_ENABLED
	CAGE_LOG(SeverityEnum::Info, "test", "profiling is enabled at compile time");
#else
	CAGE_LOG(SeverityEnum::Info, "test", "profiling was disabled at compile time");
#endif // CAGE_PROFILING_ENABLED

	{
		// this causes warnings in thread sanitizer, we will keep the profiler disabled and test the interface only
		//CAGE_TESTCASE("enabling profiling in configuration");
		//configSetBool("cage/profiling/autoStartClient", false);
		//configSetBool("cage/profiling/enabled", true);
	}

	{
		CAGE_TESTCASE("scope");
		ProfilingScope profiling("scoped test");
		profiling.set("dynamic description");
		someMeaninglessWork();
	}

	{
		CAGE_TESTCASE("separate event");
		auto evt = profilingEventBegin("separate event test");
		evt.set("dynamic description");
		someMeaninglessWork();
		profilingEventEnd(evt);
	}
}
