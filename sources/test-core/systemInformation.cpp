#include "main.h"
#include <cage-core/systemInformation.h>

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
}
