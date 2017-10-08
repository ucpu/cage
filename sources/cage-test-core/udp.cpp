#include <algorithm>

#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

namespace
{
	class senderClass
	{
	public:
		senderClass(uint32 idx) : idx(idx), snd(0), rec(0)
		{
			thread = newThread(delegate<void()>().bind<senderClass, &senderClass::process>(this), string() + "udp test sender " + idx);
		}

		void process()
		{
			peer = newUdpConnection("localhost", 4242, 1);

			while (rec < 1000)
			{
				if (read())
					continue;

				if (snd - rec < 10)
				{
					uint32 m = rand() % 5 + 1;
					for (uint32 i = 0; i < m; i++)
					{
						uint32 l = generate(snd++);
						peer->write(buf1, l, 0, true);
					}
				}

				threadSleep(500 + idx * 500);
			}

			buf1[0] = 0;
			detail::memcpy(buf1, "done", 4);
			peer->write(buf1, 4, 0, true);

			while (read());
		}

		bool read()
		{
			uintPtr a = peer->available();
			CAGE_ASSERT_RUNTIME(a < 1000);
			if (a > 0)
			{
				uint32 l = generate(rec++);
				CAGE_TEST(l == a);
				peer->read(buf2, a);
				for (uint32 i = 0; i < a; i++)
					CAGE_TEST(buf1[i] == buf2[a - i - 1]);
				return true;
			}
			return false;
		}

		uint32 generate(uint32 n)
		{
			uint32 len = n % 10 + idx + 5;
			for (uint32 i = 0; i < len; i++)
				buf1[i] = (i + n + idx) % 20;
			return len;
		}

		uint32 snd;
		uint32 rec;
		uint32 idx;
		char buf1[1000], buf2[1000];
		holder<threadClass> thread;
		holder<udpConnectionClass> peer;
	};

	class receiverClass
	{
	public:
		receiverClass(holder<udpConnectionClass> peer) : peer(peer)
		{}

		const bool process()
		{
			uintPtr a = peer->available();
			CAGE_ASSERT_RUNTIME(a < 1000);
			if (a == 0)
				return false;
			peer->read(buf, a);
			for (uint32 i = 0; i < a / 2; i++)
				std::swap(buf[i], buf[a - i - 1]);
			peer->write(buf, a, 0, true);
			return a == 4 && string(buf, numeric_cast<uint32>(a)) == "enod";
		}

		char buf[1000];
		holder<udpConnectionClass> peer;
	};
}

void testUdp()
{
	CAGE_TESTCASE("udp");

	static const uint32 connections = 5;

	holder<udpServerClass> server = newUdpServer(4242, 1, connections);

	holder<senderClass> senders[connections];
	for (uint32 i = 0; i < connections; i++)
		senders[i] = detail::systemArena().createHolder<senderClass>(i);

	holder<receiverClass> receivers[connections];
	uint32 recsCount = 0;
	uint32 done = 0;
	while (done < connections)
	{
		for (uint32 i = 0; i < recsCount; i++)
			done += receivers[i]->process();

		holder<udpConnectionClass> r = server->accept();
		if (!r)
		{
			threadSleep(1000);
			continue;
		}
		receivers[recsCount++] = detail::systemArena().createHolder<receiverClass>(r);
	}
}