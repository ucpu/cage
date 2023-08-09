#include <list>

#include "common.h"

#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/core.h>
#include <cage-core/networkGinnel.h>

struct Thr
{
	Holder<Thread> thr;
	Holder<Conn> conn;
	Runner runner;
	bool done = false;

	Thr(Holder<Conn> conn) : conn(std::move(conn)) { thr = newThread(Delegate<void()>().bind<Thr, &Thr::entry>(this), "thr"); }

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
	CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "running in server mode");

	ConfigUint32 port("port");
	CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "port: " + (uint32)port);

	Holder<GinnelServer> server = newGinnelServer(numeric_cast<uint16>((uint32)port));
	bool hadConnection = false;
	std::list<Thr> thrs;
	Runner runner;
	while (true)
	{
		while (auto a = server->accept())
		{
			CAGE_LOG(SeverityEnum::Info, "server", "connection accepted");
			hadConnection = true;
			thrs.emplace_back(newConn(std::move(a)));
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
