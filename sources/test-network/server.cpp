#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

#include <list>

using namespace cage;

#include "conn.h"
#include "runner.h"

struct thrStruct
{
	holder<threadHandle> thr;
	holder<connClass> conn;
	runnerStruct runner;
	bool done;

	thrStruct(holder<connClass> conn) : conn(templates::move(conn)), done(false)
	{
		thr = newThread(delegate<void()>().bind<thrStruct, &thrStruct::entry>(this), "thr");
	}

	void entry()
	{
		try
		{
			while (!conn->process())
				runner.step();
		}
		catch (...)
		{
			// nothing
		}
		done = true;
	}
};

void runServer()
{
	CAGE_LOG(severityEnum::Info, "config", string() + "running in server mode");

	configUint32 port("port");
	CAGE_LOG(severityEnum::Info, "config", string() + "port: " + (uint32)port);

	holder<udpServer> server = newUdpServer(numeric_cast<uint16>((uint32)port));
	bool hadConnection = false;
	std::list<thrStruct> thrs;
	runnerStruct runner;
	while (true)
	{
		while (true)
		{
			auto a = server->accept();
			if (a)
			{
				CAGE_LOG(severityEnum::Info, "server", "connection accepted");
				hadConnection = true;
				thrs.emplace_back(newConn(templates::move(a)));
			}
			else
				break;
		}
		if (hadConnection)
		{
			auto it = thrs.begin();
			while (it != thrs.end())
			{
				if (it->done)
				{
					CAGE_LOG(severityEnum::Info, "server", "connection finished");
					it = thrs.erase(it);
				}
				else
					it++;
			}
			if (thrs.empty())
				break;
		}
		runner.step();
	}
}
