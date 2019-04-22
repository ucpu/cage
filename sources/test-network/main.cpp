#include <exception>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/cmdOptions.h>
#include <cage-core/program.h>
#include <cage-core/concurrent.h>
#include <cage-core/config.h>

using namespace cage;

void runServer();
void runClient();

namespace
{
	struct runnerStruct
	{
		holder<threadClass> thr;
		string cmd;
		uint32 name;

		runnerStruct(uint32 name, const string &cmd) : cmd(cmd), name(name)
		{
			thr = newThread(delegate<void()>().bind<runnerStruct, &runnerStruct::run>(this), name);
		}
	
		~runnerStruct()
		{
			thr->wait();
		}

		void run()
		{
			holder<programClass> prg = newProgram(cmd);
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
		uint16 port = randomRange(1025u, 65535u);
		runnerStruct runnerClient1(1, string() + "cage-test-network -n network-test-client-1 -c -p " + port + " -l " + randomRange(0.0, 0.001));
		runnerStruct runnerClient2(2, string() + "cage-test-network -n network-test-client-2 -c -p " + port + " -l " + randomRange(0.0, 0.001));
		runnerStruct runnerServer0(0, string() + "cage-test-network -n network-test-server-0 -s -p " + port + " -l " + randomRange(0.0, 0.001));
		runnerStruct runnerClient3(3, string() + "cage-test-network -n network-test-client-3 -c -p " + port + " -l " + randomRange(0.0, 0.001));
		runnerStruct runnerClient4(4, string() + "cage-test-network -n network-test-client-4 -c -p " + port + " -l " + randomRange(0.0, 0.001));
		runnerStruct runnerClient5(5, string() + "cage-test-network -n network-test-client-5 -c -p " + port + " -l " + randomRange(0.0, 0.2));
	}

	void initializeSecondaryLog(const string &path)
	{
		static holder<logOutputPolicyFileClass> *secondaryLogFile = new holder<logOutputPolicyFileClass>(); // intentional leak
		static holder<loggerClass> *secondaryLog = new holder<loggerClass>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLogOutputPolicyFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<logOutputPolicyFileClass, &logOutputPolicyFileClass::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatPolicyFileShort>();
	}
}

int main(int argc, const char *args[])
{
	try
	{
		// log to console
		holder<loggerClass> log1 = newLogger();
		log1->filter.bind<logFilterPolicyPass>();
		log1->format.bind<logFormatPolicyConsole>();
		log1->output.bind<logOutputPolicyStdOut>();

		if (argc == 1)
		{
			initializeSecondaryLog("network-test-manager.log");
			runManager();
			return 0;
		}

		holder<cmdOptionsClass> cmd = newCmdOptions(argc, args, "n:sca:p:l:");
		string name;
		configString address("address", "localhost");
		configUint32 port("port", 42789);
		configFloat packetLoss("cage-core.udp.simulatedPacketLoss");
		bool modeServer = false;
		bool modeClient = false;
		while (cmd->next())
		{
			switch (cmd->option())
			{
			case 'n':
				name = cmd->argument();
				break;
			case 's':
				modeServer = true;
				break;
			case 'c':
				modeClient = true;
				break;
			case 'a':
				address = cmd->argument();
				break;
			case 'p':
				port = string(cmd->argument()).toUint32();
				if (port <= 1024 || port >= 65536)
					CAGE_THROW_ERROR(exception, "invalid port");
				break;
			case 'l':
				packetLoss = string(cmd->argument()).toFloat();
				if (packetLoss < 0 || packetLoss > 1)
					CAGE_THROW_ERROR(exception, "invalid packet loss");
				break;
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
