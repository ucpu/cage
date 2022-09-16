#include "main.h"

#include <cage-core/systemInformation.h>

namespace
{
	struct LargeObject : private Immovable
	{
		static constexpr uint64 Count = 16 * 1024 * 1024;
		uint64 arr[Count] = {};

		LargeObject()
		{
			uint64 v = 13;
			for (uint64 *p = arr; p < arr + Count; p++)
				*p = v++;
		}

		void validate()
		{
			uint64 v = 13;
			for (uint64 *p = arr; p < arr + Count; p++)
				CAGE_TEST(*p == v++);
		}
	};
}

void testSystemInformation()
{
	CAGE_TESTCASE("system information");

	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "system: " + systemName());
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "user: " + userName());
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "host: " + hostName());
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "processors count: " + processorsCount());
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "processor name: " + processorName());
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "processor speed: " + (processorClockSpeed() / 1000 / 1000) + " MHz");
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "memory capacity: " + (memoryCapacity() / 1024 / 1024) + " MB");
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "memory available: " + (memoryAvailable() / 1024 / 1024) + " MB");
	CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "memory used: " + (memoryUsed() / 1024 / 1024) + " MB");

	{
		CAGE_TESTCASE("current process memory used");
		const uint64 a = memoryUsed() / 1024 / 1024;
		Holder<LargeObject> h = systemMemory().createHolder<LargeObject>();
		const uint64 b = memoryUsed() / 1024 / 1024;
		h->validate();
		h.clear();
		const uint64 c = memoryUsed() / 1024 / 1024;
		CAGE_LOG(SeverityEnum::Info, "info", Stringizer() + "a: " + a + ", b: " + b + ", c: " + c + " MB");
		CAGE_TEST(a > 10);
		CAGE_TEST(b > a + 10);
	}
}
