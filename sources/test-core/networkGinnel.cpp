#include <algorithm>
#include <atomic>
#include <vector>

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/networkGinnel.h>

namespace
{
	std::atomic<uint32> connectionsLeft;

	class ServerImpl
	{
	public:
		Holder<GinnelServer> udp = newGinnelServer(3210);
		std::vector<Holder<GinnelConnection>> conns;
		uint64 lastTime = applicationTime();
		bool hadConnection = false;

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
				while (true)
				{
					uint32 ch;
					bool r;
					Holder<PointerRange<char>> b = c->read(ch, r);
					if (!b)
						break;
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
		Holder<GinnelConnection> udp;
		std::vector<MemoryBuffer> sends;
		uint32 si = 0, ri = 0;

		ClientImpl()
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
			udp = newGinnelConnection("localhost", 3210, 0);
		}

		~ClientImpl()
		{
			if (udp)
			{
				const auto &s = udp->statistics();
				CAGE_LOG(SeverityEnum::Info, "udp-stats", Stringizer() + "ping: " + (s.roundTripDuration / 1000) + " ms, receiving: " + (s.bpsReceived() / 1024) + " KB/s, sending: " + (s.bpsSent() / 1024) + " KB/s");
			}
			connectionsLeft--;
		}

		bool service()
		{
			while (true)
			{
				Holder<PointerRange<char>> r = udp->read();
				if (!r)
					break;
				MemoryBuffer &b = sends[ri++];
				CAGE_TEST(r.size() == b.size());
				CAGE_TEST(detail::memcmp(r.data(), b.data(), b.size()) == 0);
				CAGE_LOG(SeverityEnum::Info, "udp-test", Stringizer() + "progress: " + ri + " / " + sends.size());
			}
			if (si < ri + 2 && si < sends.size())
			{
				MemoryBuffer &b = sends[si++];
				udp->write(b, 5, true);
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

void testNetworkGinnel()
{
	CAGE_TESTCASE("network ginnel");
#ifdef CAGE_SYSTEM_MAC
	CAGE_LOG(SeverityEnum::Warning, "tests", "skipping the test - macos");
	return;
#endif
	Holder<Thread> server = newThread(Delegate<void()>().bind<&ServerImpl::entry>(), "server");
	std::vector<Holder<Thread>> clients;
	clients.resize(3);
	uint32 index = 0;
	for (auto &c : clients)
		c = newThread(Delegate<void()>().bind<&ClientImpl::entry>(), Stringizer() + "client " + (index++));
	server->wait();
	for (auto &c : clients)
		c->wait();
}
