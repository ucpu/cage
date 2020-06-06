#include <cage-core/logger.h>

#include "database.h"

bool consoleLogFilter(const cage::detail::LoggerInfo &info)
{
	return info.severity >= SeverityEnum::Error || string(info.component) == "exception" || string(info.component) == "asset" || string(info.component) == "verdict";
}

int main(int argc, const char *args[])
{
	try
	{
		Holder<Logger> conLog = newLogger();
		conLog->filter.bind<&consoleLogFilter>();
		conLog->format.bind<&logFormatConsole>();
		conLog->output.bind<&logOutputStdOut>();

		configParseCmd(argc, args);

		CAGE_LOG(SeverityEnum::Info, "database", "start");
		start();

		if (configListening)
		{
			CAGE_LOG(SeverityEnum::Info, "database", "waiting for changes");
			try
			{
				listen();
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Info, "database", "stopped");
			}
		}

		CAGE_LOG(SeverityEnum::Info, "database", "end");

		return verdict() ? 0 : -1;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
