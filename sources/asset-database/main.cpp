#include <set>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>

using namespace cage;

#include "config.h"
#include "processor.h"

bool consoleLogFilter(const cage::detail::loggerInfo &info)
{
	return info.severity >= severityEnum::Error || string(info.component) == "exception" || string(info.component) == "asset" || string(info.component) == "verdict";
}

int main(int argc, const char *args[])
{
	holder<loggerClass> conLog = newLogger();
	conLog->filter.bind<&consoleLogFilter>();
	conLog->format.bind<&logFormatPolicyConsole>();
	conLog->output.bind<&logOutputPolicyStdOut>();

	configParseCmd(argc, args);

	CAGE_LOG(severityEnum::Info, "database", "start");
	start();

	if (configListening)
	{
		CAGE_LOG(severityEnum::Info, "database", "waiting for changes");
		try
		{
			listen();
		}
		catch (...)
		{
			CAGE_LOG(severityEnum::Info, "database", "stopped");
		}
	}

	CAGE_LOG(severityEnum::Info, "database", "end");

	return verdict() ? 0 : -1;
}
