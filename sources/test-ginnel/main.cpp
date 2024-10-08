#include "common.h"

#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/process.h>

void runServer();
void runClient();

namespace
{
	struct Run : private Immovable
	{
		Holder<Thread> thr;
		String name;
		String cmd;

		explicit Run(const String &name, const String &cmd) : name(name), cmd(cmd) { thr = newThread(Delegate<void()>().bind<Run, &Run::thrEntry>(this), name); }

		void thrEntry()
		{
			Holder<Process> prg = newProcess(cmd);
			{
				while (true)
				{
					String s;
					try
					{
						detail::OverrideBreakpoint ob;
						s = prg->readLine();
					}
					catch (const ProcessPipeEof &)
					{
						break;
					}
					CAGE_LOG(SeverityEnum::Info, "manager", name + ": " + s);
				}
			}
			int res = prg->wait();
			if (res != 0)
				CAGE_LOG(SeverityEnum::Info, "manager", Stringizer() + name + ": process ended with code: " + res);
		}
	};

	void runManager()
	{
		Run runnerClient1("1", "cage-test-ginnel -n ginnel-test-1 -c");
		Run runnerServerS("S", "cage-test-ginnel -n ginnel-test-S -s");
		Run runnerClient2("2", "cage-test-ginnel -n ginnel-test-2 -c");
		Run runnerClient3("3", "cage-test-ginnel -n ginnel-test-3 -c");
	}

	void initializeSecondaryLog(const String &path)
	{
		static Holder<LoggerOutputFile> *secondaryLogFile = new Holder<LoggerOutputFile>(); // intentional leak
		static Holder<Logger> *secondaryLog = new Holder<Logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatFileShort>();
	}

#ifdef CAGE_DEBUG
	constexpr uint64 MaxBytesPerSecond = 256 * 1024;
#else
	constexpr uint64 MaxBytesPerSecond = 10 * 1024 * 1024;
#endif // CAGE_DEBUG
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	try
	{
		if (argc == 1)
		{
			initializeSecondaryLog("ginnel-test-manager.log");
			runManager();
			return 0;
		}

		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		ConfigString address("address", "localhost");
		ConfigUint32 port("port", 42789);
		ConfigUint64 maxBytesPerSecond("maxBytesPerSecond");
		const bool modeServer = cmd->cmdBool('s', "server", false);
		const bool modeClient = cmd->cmdBool('c', "client", false);
		const String name = cmd->cmdString('n', "name", "");
		address = cmd->cmdString('a', "address", address);
		port = cmd->cmdUint32('p', "port", port);
		if (port <= 1024 || port >= 65536)
			CAGE_THROW_ERROR(Exception, "invalid port");
		maxBytesPerSecond = cmd->cmdUint64('l', "limit", MaxBytesPerSecond);
		cmd->checkUnusedWithHelp();
		cmd.clear();

		if (modeServer == modeClient)
			CAGE_THROW_ERROR(Exception, "invalid mode (exactly one of -s or -c must be set)");

		if (!name.empty())
			initializeSecondaryLog(name + ".log");

		if (modeServer)
			runServer();
		if (modeClient)
			runClient();

		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
