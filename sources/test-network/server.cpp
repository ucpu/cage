#include <cage-core/core.h>
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "conn.h"
#include "runner.h"

#include <list>

struct Thr
{
	Holder<Thread> thr;
	Holder<Conn> conn;
	Runner runner;
	bool done;

	Thr(Holder<Conn> conn) : conn(templates::move(conn)), done(false)
	{
		thr = newThread(Delegate<void()>().bind<Thr, &Thr::entry>(this), "thr");
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
	CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "running in server mode");

	ConfigUint32 port("port");
	CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "port: " + (uint32)port);

	Holder<UdpServer> server = newUdpServer(numeric_cast<uint16>((uint32)port));
	bool hadConnection = false;
	std::list<Thr> thrs;
	Runner runner;
	while (true)
	{
		while (true)
		{
			auto a = server->accept();
			if (a)
			{
				CAGE_LOG(SeverityEnum::Info, "server", "connection accepted");
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
					CAGE_LOG(SeverityEnum::Info, "server", "connection finished");
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
