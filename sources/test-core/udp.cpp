#include <vector>
#include <algorithm>
#include <atomic>

#include "main.h"
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/math.h> // random
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/identifier.h> // generateRandomData

namespace
{
	std::atomic<uint32> connectionsLeft;

	class serverImpl
	{
	public:
		holder<udpServerClass> udp;
		std::vector<holder<udpConnectionClass>> conns;
		uint64 lastTime;
		bool hadConnection;

		serverImpl() : lastTime(getApplicationTime()), hadConnection(false)
		{
			udp = newUdpServer(3210);
		}

		bool service()
		{
			while (true)
			{
				auto c = udp->accept();
				if (c)
				{
					conns.push_back(templates::move(c));
					hadConnection = true;
				}
				else
					break;
			}

			for (auto &c : conns)
			{
				while (c->available())
				{
					uint32 ch;
					bool r;
					memoryBuffer b = c->read(ch, r);
					c->write(b, ch, r); // just repeat back the same message
					lastTime = getApplicationTime();
				}
				c->update();
			}

			return connectionsLeft > 0 || !hadConnection;
		}

		static void entry()
		{
			serverImpl srv;
			while (srv.service())
				threadSleep(1000);
		}
	};

	class clientImpl
	{
	public:
		holder<udpConnectionClass> udp;

		std::vector<memoryBuffer> sends;
		uint32 si, ri;

		clientImpl() : si(0), ri(0)
		{
			connectionsLeft++;
			for (uint32 i = 0; i < 3; i++)
			{
				memoryBuffer b(random(500000, 1000000));
				privat::generateRandomData((uint8*)b.data(), numeric_cast<uint32>(b.size()));
				sends.push_back(templates::move(b));
			}
			udp = newUdpConnection("localhost", 3210, cage::random() < 0.5 ? 3000000 : 0);
		}

		~clientImpl()
		{
			connectionsLeft--;
		}

		bool service()
		{
			while (udp->available())
			{
				memoryBuffer r = udp->read();
				memoryBuffer &b = sends[ri++];
				CAGE_TEST(r.size() == b.size());
				CAGE_TEST(detail::memcmp(r.data(), b.data(), b.size()) == 0);
				CAGE_LOG(severityEnum::Info, "udp-test", string() + "progress: " + ri + " / " + sends.size());
			}
			if (si < ri + 2 && si < sends.size())
			{
				memoryBuffer &b = sends[si++];
				udp->write(b, 13, true);
			}
			udp->update();
			return ri < sends.size();
		}

		static void entry()
		{
			clientImpl cl;
			while (cl.service())
				threadSleep(1000);
		}
	};
}

void testUdp()
{
	CAGE_TESTCASE("udp");

	configSetUint32("cage-core.udp.logLevel", 2);
	configSetUint32("cage-core.udp.packetsPerService", 1);
	configSetFloat("cage-core.udp.simulatedPacketLoss", 0.1);

	holder<threadClass> server = newThread(delegate<void()>().bind<&serverImpl::entry>(), "server");
	std::vector<holder<threadClass>> clients;
	clients.resize(2);
	uint32 index = 0;
	for (auto &c : clients)
		c = newThread(delegate<void()>().bind<&clientImpl::entry>(), string() + "client " + (index++));
	server->wait();
	for (auto &c : clients)
		c->wait();
}
