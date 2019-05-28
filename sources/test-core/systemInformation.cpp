#include "main.h"
#include <cage-core/systemInformation.h>

void testSystemInformation()
{
	CAGE_TESTCASE("system information");

	CAGE_LOG(severityEnum::Info, "info", string() + "system: " + systemName());
	CAGE_LOG(severityEnum::Info, "info", string() + "user: " + userName());
	CAGE_LOG(severityEnum::Info, "info", string() + "host: " + hostName());
	CAGE_LOG(severityEnum::Info, "info", string() + "processors count: " + processorsCount());
	CAGE_LOG(severityEnum::Info, "info", string() + "processor name: " + processorName());
	CAGE_LOG(severityEnum::Info, "info", string() + "processor speed: " + (processorClockSpeed() / 1000 / 1000) + " MHz");
	CAGE_LOG(severityEnum::Info, "info", string() + "memory capacity: " + (memoryCapacity() / 1024 / 1024) + " MB");
	CAGE_LOG(severityEnum::Info, "info", string() + "memory available: " + (memoryAvailable() / 1024 / 1024) + " MB");
}
