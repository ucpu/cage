#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

#include <list>

using namespace cage;

#include "conn.h"

void runServer()
{
	CAGE_LOG(severityEnum::Info, "config", string() + "running in server mode");

	configUint32 port("port");
	CAGE_LOG(severityEnum::Info, "config", string() + "port: " + (uint32)port);

	holder<udpServerClass> server = newUdpServer(numeric_cast<uint16>((uint32)port));
	bool hadConnection = false;
	std::list<holder<connClass>> conns;
	uint64 time = getApplicationTime();
	while (true)
	{
		while (true)
		{
			auto a = server->accept();
			if (a)
			{
				CAGE_LOG(severityEnum::Info, "server", "connection accepted");
				hadConnection = true;
				conns.emplace_back(newConn(templates::move(a)));
			}
			else
				break;
		}
		if (hadConnection)
		{
			auto it = conns.begin();
			while (it != conns.end())
			{
				if ((*it)->process())
				{
					CAGE_LOG(severityEnum::Info, "server", "connection finished");
					it = conns.erase(it);
				}
				else
					it++;
			}
			if (conns.empty())
				break;
		}
		{
			uint64 t = getApplicationTime();
			sint64 s = time + timeStep - t;
			if (s > 0)
				threadSleep(s);
			else
				CAGE_LOG(severityEnum::Warning, "server", "cannot keep up");
			time += timeStep;
		}
	}
}
