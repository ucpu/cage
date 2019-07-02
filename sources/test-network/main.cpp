#include <exception>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/configIni.h>
#include <cage-core/program.h>
#include <cage-core/concurrent.h>
#include <cage-core/config.h>

using namespace cage;

void runServer();
void runClient();

namespace
{
	struct runStruct
	{
		holder<thread> thr;
		string cmd;
		uint32 name;

		runStruct(uint32 name, const string &cmd) : cmd(cmd), name(name)
		{
			thr = newThread(delegate<void()>().bind<runStruct, &runStruct::run>(this), name);
		}
	
		~runStruct()
		{
			thr->wait();
		}

		void run()
		{
			holder<program> prg = newProgram(cmd);
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
					CAGE_LOG(severityEnum::Info, "manager", string() + name + ": " + s);
				}
			}
			int res = prg->wait();
			if (res != 0)
				CAGE_LOG(severityEnum::Info, "manager", string() + name + ": program ended with code: " + res);
		}
	};

	void runManager()
	{
		runStruct runnerClient1(1, string() + "cage-test-network -n network-test-1 -c");
		runStruct runnerClient2(2, string() + "cage-test-network -n network-test-2 -c -l 0.2");
		runStruct runnerServer0(0, string() + "cage-test-network -n network-test-0 -s");
		runStruct runnerClient3(3, string() + "cage-test-network -n network-test-3 -c");
	}

	void initializeSecondaryLog(const string &path)
	{
		static holder<logOutputFile> *secondaryLogFile = new holder<logOutputFile>(); // intentional leak
		static holder<logger> *secondaryLog = new holder<logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLogOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<logOutputFile, &logOutputFile::output>(secondaryLogFile->get());
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

		holder<configIni> cmd = newConfigIni();
		cmd->parseCmd(argc, args);
		string name;
		configString address("address", "localhost");
		configUint32 port("port", 42789);
		configFloat packetLoss("cage-core.udp.simulatedPacketLoss");
		configUint32 confMessages("messages", 150);
		bool modeServer = false;
		bool modeClient = false;
		for (string option : cmd->sections())
		{
			if (option == "n" || option == "name")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				name = cmd->get(option, "0");
			}
			else if (option == "s" || option == "server")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				modeServer = cmd->getBool(option, "0");
			}
			else if (option == "c" || option == "client")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				modeClient = cmd->getBool(option, "0");
			}
			else if (option == "a" || option == "address")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				address = cmd->get(option, "0");
			}
			else if (option == "p" || option == "port")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				port = cmd->getUint32(option, "0");
				if (port <= 1024 || port >= 65536)
					CAGE_THROW_ERROR(exception, "invalid port");
			}
			else if (option == "l" || option == "packetLoss")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				packetLoss = cmd->getFloat(option, "0");
				if (packetLoss < 0 || packetLoss > 1)
					CAGE_THROW_ERROR(exception, "invalid packet loss");
			}
			else if (option == "m" || option == "messages")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				confMessages = cmd->getUint32(option, "0");
			}
			else
			{
				CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
				CAGE_THROW_ERROR(exception, "unknown option");
			}
		}

		if (modeServer == modeClient)
			CAGE_THROW_ERROR(exception, "invalid mode (exactly one of -s or -c must be set)");

		if (!name.empty())
			initializeSecondaryLog(name + ".log");
		CAGE_LOG(severityEnum::Info, "config", string() + "packet loss: " + (float)packetLoss);

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
		CAGE_LOG(severityEnum::Error, "exception", string() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "unknown exception");
	}
	return 1;
}
