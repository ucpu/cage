#include <exception>

#include <cage-core/core.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/configIni.h>
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
		holder<threadHandle> thr;
		string cmd;
		uint32 name;

		runStruct(uint32 name, const string &cmd) : cmd(cmd), name(name)
		{
			thr = newThread(delegate<void()>().bind<runStruct, &runStruct::run>(this), string(name));
		}
	
		~runStruct()
		{
			thr->wait();
		}

		void run()
		{
			holder<processHandle> prg = newProcess(cmd);
			{
				while (true)
				{
					string s;
					{
						detail::overrideBreakpoint ob;
						s = prg->readLine();
					}
					if (s.empty())
						break;
					CAGE_LOG(severityEnum::Info, "manager", stringizer() + name + ": " + s);
				}
			}
			int res = prg->wait();
			if (res != 0)
				CAGE_LOG(severityEnum::Info, "manager", stringizer() + name + ": process ended with code: " + res);
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
		static holder<loggerOutputFile> *secondaryLogFile = new holder<loggerOutputFile>(); // intentional leak
		static holder<logger> *secondaryLog = new holder<logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<loggerOutputFile, &loggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatFileShort>();
	}
}

int main(int argc, const char *args[])
{
	try
	{
		// log to console
		holder<logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		if (argc == 1)
		{
			initializeSecondaryLog("network-test-manager.log");
			runManager();
			return 0;
		}

		holder<configIni> cmd = newConfigIni(argc, args);
		configString address("address", "localhost");
		configUint32 port("port", 42789);
		configFloat packetLoss("cage/udp/simulatedPacketLoss");
		configUint32 confMessages("messages", 150);
		bool modeServer = cmd->cmdBool('s', "server", false);
		bool modeClient = cmd->cmdBool('c', "client", false);
		string name = cmd->cmdString('n', "name", "");
		address = cmd->cmdString('a', "address", address);
		port = cmd->cmdUint32('p', "port", port);
		if (port <= 1024 || port >= 65536)
			CAGE_THROW_ERROR(exception, "invalid port");
		packetLoss = cmd->cmdFloat('l', "packetLoss", packetLoss);
		if (packetLoss < 0 || packetLoss > 1)
			CAGE_THROW_ERROR(exception, "invalid packet loss");
		confMessages = cmd->cmdUint32('m', "messages", confMessages);
		cmd->checkUnused();
		cmd.clear();

		if (modeServer == modeClient)
			CAGE_THROW_ERROR(exception, "invalid mode (exactly one of -s or -c must be set)");

		if (!name.empty())
			initializeSecondaryLog(name + ".log");
		CAGE_LOG(severityEnum::Info, "config", stringizer() + "packet loss: " + (float)packetLoss);

		if (modeServer)
			runServer();
		if (modeClient)
			runClient();

		return 0;
	}
	catch (const cage::exception &)
	{
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Error, "exception", stringizer() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "unknown exception");
	}
	return 1;
}
