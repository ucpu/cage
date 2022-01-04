#include "main.h"
#include <cage-core/profiling.h>
#include <cage-core/config.h>

namespace
{
	void someMeaninglessWork()
	{
		volatile int v = 0;
		for (uint32 i = 0; i < 1000; i++)
			v += i;
	}
}

void testProfiling()
{
	CAGE_TESTCASE("profiling");

#ifdef CAGE_PROFILING_ENABLED
	CAGE_LOG(SeverityEnum::Info, "test", "profiling is enabled");
#else
	CAGE_LOG(SeverityEnum::Info, "test", "profiling was disabled at compile time");
#endif // CAGE_PROFILING_ENABLED

	{
		CAGE_TESTCASE("enabling profiling in configuration");
		configSetBool("cage/profiling/enabled", true);
	}

	{
		CAGE_TESTCASE("scope");
		ProfilingScope profiling("profiling-scope-test", "test");
		someMeaninglessWork();
	}

	{
		CAGE_TESTCASE("object");
		struct Object
		{
			ProfilingScope profiling;

			Object() : profiling("profiling-object-test", "test", ProfilingTypeEnum::Object)
			{}
		};

		Object obj;
		obj.profiling.snapshot("\"value\":0");
		someMeaninglessWork();
		obj.profiling.snapshot("\"value\":1, \"hello\":\"world\"");
	}

	{
		CAGE_TESTCASE("markers");
		profilingMarker("before", "test");
		someMeaninglessWork();
		profilingMarker("after", "test");
	}

	{
		CAGE_TESTCASE("duration event");
		profilingEvent(42, "duration-event", "test");
	}

	{
		CAGE_TESTCASE("separate event");
		const auto evt = profilingEventBegin("separate-event", "test");
		someMeaninglessWork();
		profilingEventEnd(evt);
	}

	{
		CAGE_TESTCASE("separate event with value");
		const auto evt = profilingEventBegin("separate-event", "test");
		someMeaninglessWork();
		profilingEventEnd(evt, "\"value\":42");
	}

	{
		CAGE_TESTCASE("async");
		const auto evt = profilingEventBegin("async-event", "test", ProfilingTypeEnum::Async);
		someMeaninglessWork();
		profilingEventEnd(evt);
	}

	{
		CAGE_TESTCASE("flow");
		const auto evt = profilingEventBegin("flow-event", "test", ProfilingTypeEnum::Flow);
		someMeaninglessWork();
		profilingEventEnd(evt);
	}
}