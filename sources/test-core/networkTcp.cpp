#include "main.h"
#include <cage-core/networkTcp.h>
#include <cage-core/concurrent.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/math.h>

namespace
{
	struct LoremIpsumTest
	{
		LoremIpsumTest()
		{
			Holder<TcpServer> server = newTcpServer(4241);
			Holder<Thread> thr = newThread(Delegate<void()>().bind<LoremIpsumTest, &LoremIpsumTest::client>(this), "tcp client");
			Holder<TcpConnection> conn;
			while (!conn)
			{
				threadYield();
				conn = server->accept();
			}
			server.clear(); // listen no more
			recv(conn, "lorem");
			send(conn, "ipsum");
			recv(conn, "dolor");
			send(conn, "sit");
			recv(conn, "amet");
			CAGE_TEST_THROWN(conn->readLine()); // connection broken
		}

		void client()
		{
			Holder<TcpConnection> conn = newTcpConnection("localhost", 4241);
			send(conn, "lorem");
			recv(conn, "ipsum");
			send(conn, "dolor");
			recv(conn, "sit");
			send(conn, "amet");
		}

		void recv(Holder<TcpConnection> &conn, const String &msg)
		{
			while (randomChance() < 0.2)
				threadYield();
			String line = conn->readLine();
			CAGE_TEST(line == msg);
		}

		void send(Holder<TcpConnection> &conn, const String &msg)
		{
			while (randomChance() < 0.2)
				threadYield();
			conn->writeLine(msg);
		}
	};

	struct EchoTest
	{
		EchoTest()
		{
			Holder<TcpServer> server = newTcpServer(4243);
			Holder<Thread> thr[4];
			for (int i = 0; i < 4; i++)
				thr[i] = newThread(Delegate<void()>().bind<EchoTest, &EchoTest::client>(this), "tcp client");
			Holder<Echo> echos[4];
			for (int i = 0; i < 4; i++)
			{
				while (true)
				{
					Holder<TcpConnection> c = server->accept();
					if (c)
					{
						echos[i] = systemMemory().createHolder<Echo>(std::move(c));
						break;
					}
					threadYield();
				}
			}
		}

		MemoryBuffer generateRandomBuffer()
		{
			const uintPtr sz = randomRange(100 * 1024, 5 * 1024 * 1024);
			MemoryBuffer b;
			Serializer ser(b);
			for (uintPtr i = 0; i < sz; i++)
				ser << randomRange(-100, 100);
			return b;
		}

		void client()
		{
			Holder<TcpConnection> conn = newTcpConnection("localhost", 4243);
			MemoryBuffer out = generateRandomBuffer();
			Deserializer des(out);
			MemoryBuffer in;
			Serializer ser(in);

			while (in.size() < out.size())
			{
				{ // read
					ser.write(conn->readAll());
				}
				{ // write
					const uintPtr a = des.available();
					if (a)
					{
						const uintPtr s = a > 10 ? randomRange(uintPtr(5), min(a, uintPtr(1000))) : a;
						conn->write(des.read(s));
					}
				}
				threadYield();
			}

			CAGE_TEST(in.size() == out.size());
			CAGE_TEST(detail::memcmp(in.data(), out.data(), in.size()) == 0);
		}

		struct Echo
		{
			Holder<TcpConnection> conn;
			Holder<Thread> thr;

			Echo(Holder<TcpConnection> conn) : conn(std::move(conn))
			{
				thr = newThread(Delegate<void()>().bind<Echo, &Echo::responder>(this), "tcp responder");
			}

			void responder()
			{
				try
				{
					while (true)
					{
						conn->write(conn->readAll());
						threadYield();
					}
				}
				catch (const Disconnected &)
				{
					// ok
				}
			}
		};
	};
}

void testNetworkTcp()
{
	CAGE_TESTCASE("network tcp");

	{
		CAGE_TESTCASE("lorem ipsum");
		LoremIpsumTest test;
	}

	{
		CAGE_TESTCASE("echo");
		EchoTest test;
	}
}
