#include <cage-core/logger.h>

#include "database.h"

extern ConfigBool configListening;
extern bool verdictValue;

void configParseCmd(int argc, const char *args[]);
void start();
void listen();

bool consoleLogFilter(const cage::detail::LoggerInfo &info)
{
	return info.severity >= SeverityEnum::Error || String(info.component) == "exception" || String(info.component) == "asset" || String(info.component) == "verdict" || String(info.component) == "help";
}

int main(int argc, const char *args[])
{
	Holder<Logger> conLog = newLogger();
	conLog->filter.bind<&consoleLogFilter>();
	conLog->format.bind<&logFormatConsole>();
	conLog->output.bind<&logOutputStdOut>();

	try
	{
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

		return verdictValue ? 0 : -1;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
