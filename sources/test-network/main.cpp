#include <exception>

#include <cage-core/core.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/ini.h>
#include <cage-core/process.h>
#include <cage-core/concurrent.h>
#include <cage-core/config.h>

using namespace cage;

void runServer();
void runClient();

namespace
{
	struct runStruct
	{
		Holder<Thread> thr;
		string cmd;
		uint32 name;

		runStruct(uint32 name, const string &cmd) : cmd(cmd), name(name)
		{
			thr = newThread(Delegate<void()>().bind<runStruct, &runStruct::run>(this), string(name));
		}
	
		~runStruct()
		{
			thr->wait();
		}

		void run()
		{
			Holder<Process> prg = newProcess(cmd);
			{
				while (true)
				{
					string s;
					{
						detail::OverrideBreakpoint ob;
						s = prg->readLine();
					}
					if (s.empty())
						break;
					CAGE_LOG(SeverityEnum::Info, "manager", stringizer() + name + ": " + s);
				}
			}
			int res = prg->wait();
			if (res != 0)
				CAGE_LOG(SeverityEnum::Info, "manager", stringizer() + name + ": process ended with code: " + res);
		}
	};

	void runManager()
	{
		runStruct runnerClient1(1, "cage-test-network -n network-test-1 -c");
		runStruct runnerClient2(2, "cage-test-network -n network-test-2 -c -l 0.2");
		runStruct runnerServer0(0, "cage-test-network -n network-test-0 -s");
		runStruct runnerClient3(3, "cage-test-network -n network-test-3 -c");
	}

	void initializeSecondaryLog(const string &path)
	{
		static Holder<LoggerOutputFile> *secondaryLogFile = new Holder<LoggerOutputFile>(); // intentional leak
		static Holder<Logger> *secondaryLog = new Holder<Logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatFileShort>();
	}
}

int main(int argc, const char *args[])
{
	try
	{
		// log to console
		Holder<Logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		if (argc == 1)
		{
			initializeSecondaryLog("network-test-manager.log");
			runManager();
			return 0;
		}

		Holder<Ini> cmd = newConfigIni(argc, args);
		ConfigString address("address", "localhost");
		ConfigUint32 port("port", 42789);
		ConfigFloat packetLoss("cage/udp/simulatedPacketLoss");
		ConfigUint32 confMessages("messages", 150);
		bool modeServer = cmd->cmdBool('s', "server", false);
		bool modeClient = cmd->cmdBool('c', "client", false);
		string name = cmd->cmdString('n', "name", "");
		address = cmd->cmdString('a', "address", address);
		port = cmd->cmdUint32('p', "port", port);
		if (port <= 1024 || port >= 65536)
			CAGE_THROW_ERROR(Exception, "invalid port");
		packetLoss = cmd->cmdFloat('l', "packetLoss", packetLoss);
		if (packetLoss < 0 || packetLoss > 1)
			CAGE_THROW_ERROR(Exception, "invalid packet loss");
		confMessages = cmd->cmdUint32('m', "messages", confMessages);
		cmd->checkUnused();
		cmd.clear();

		if (modeServer == modeClient)
			CAGE_THROW_ERROR(Exception, "invalid mode (exactly one of -s or -c must be set)");

		if (!name.empty())
			initializeSecondaryLog(name + ".log");
		CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "packet loss: " + (float)packetLoss);

		if (modeServer)
			runServer();
		if (modeClient)
			runClient();

		return 0;
	}
	catch (const cage::Exception &)
	{
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(SeverityEnum::Error, "exception", stringizer() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(SeverityEnum::Error, "exception", "unknown exception");
	}
	return 1;
}
