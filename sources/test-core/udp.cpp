#include "main.h"
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/math.h> // random
#include <cage-core/memoryBuffer.h>
#include <cage-core/guid.h> // generateRandomData

#include <vector>
#include <algorithm>
#include <atomic>

namespace
{
	std::atomic<uint32> connectionsLeft;

	class ServerImpl
	{
	public:
		Holder<UdpServer> udp;
		std::vector<Holder<UdpConnection>> conns;
		uint64 lastTime;
		bool hadConnection;

		ServerImpl() : lastTime(applicationTime()), hadConnection(false)
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
					conns.push_back(std::move(c));
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
					Holder<PointerRange<char>> b = c->read(ch, r);
					c->write(b, ch, r); // just repeat back the same message
					lastTime = applicationTime();
				}
				try
				{
					c->update();
				}
				catch (const Disconnected &)
				{
					// ignore
				}
			}

			return connectionsLeft > 0 || !hadConnection;
		}

		static void entry()
		{
			ServerImpl srv;
			while (srv.service())
				threadSleep(5000);
		}
	};

	class ClientImpl
	{
	public:
		Holder<UdpConnection> udp;

		std::vector<MemoryBuffer> sends;
		uint32 si, ri;

		ClientImpl() : si(0), ri(0)
		{
			connectionsLeft++;
			for (uint32 i = 0; i < 20; i++)
			{
				uint32 cnt = randomRange(100, 10000);
				MemoryBuffer b(cnt);
				for (uint32 i = 0; i < cnt; i++)
					b.data()[i] = (char)randomRange(0u, 256u);
				sends.push_back(std::move(b));
			}
			udp = newUdpConnection("localhost", 3210, randomChance() < 0.5 ? 10000000 : 0);
		}

		~ClientImpl()
		{
			if (udp)
			{
				const auto &s = udp->statistics();
				CAGE_LOG(SeverityEnum::Info, "udp-stats", stringizer() + "rtt: " + (s.roundTripDuration / 1000) + " ms, received: " + (s.bytesReceivedTotal / 1024) + " KB, " + s.packetsReceivedTotal + " packets");
			}
			connectionsLeft--;
		}

		bool service()
		{
			while (udp->available())
			{
				Holder<PointerRange<char>> r = udp->read();
				MemoryBuffer &b = sends[ri++];
				CAGE_TEST(r.size() == b.size());
				CAGE_TEST(detail::memcmp(r.data(), b.data(), b.size()) == 0);
				CAGE_LOG(SeverityEnum::Info, "udp-test", stringizer() + "progress: " + ri + " / " + sends.size());
			}
			if (si < ri + 2 && si < sends.size())
			{
				MemoryBuffer &b = sends[si++];
				udp->write(b, 13, true);
			}
			udp->update();
			return ri < sends.size();
		}

		static void entry()
		{
			ClientImpl cl;
			while (cl.service())
				threadSleep(5000);
		}
	};
}

void testUdp()
{
	CAGE_TESTCASE("udp");

	configSetUint32("cage/udp/logLevel", 2);
	configSetFloat("cage/udp/simulatedPacketLoss", 0.1f);

	Holder<Thread> server = newThread(Delegate<void()>().bind<&ServerImpl::entry>(), "server");
	std::vector<Holder<Thread>> clients;
	clients.resize(3);
	uint32 index = 0;
	for (auto &c : clients)
		c = newThread(Delegate<void()>().bind<&ClientImpl::entry>(), stringizer() + "client " + (index++));
	server->wait();
	for (auto &c : clients)
		c->wait();
}
