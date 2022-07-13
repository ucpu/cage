#include "main.h"
#include <cage-core/networkWebsocket.h>
#include <cage-core/concurrent.h>
#include <cage-core/math.h>

namespace
{
	struct LoremIpsumTest
	{
		static constexpr const char LongText[] = "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789";

		LoremIpsumTest()
		{
			Holder<WebsocketServer> server = newWebsocketServer(4541);
			Holder<Thread> thr = newThread(Delegate<void()>().bind<LoremIpsumTest, &LoremIpsumTest::client>(this), "websocket client");
			Holder<WebsocketConnection> conn;
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
			for (uint32 i = 0; i < 100; i++)
			{
				send(conn, LongText);
				recv(conn, LongText);
			}
			send(conn, "sit");
			recv(conn, "amet");
		}

		void client()
		{
			Holder<WebsocketConnection> conn = newWebsocketConnection("localhost", 4541);
			send(conn, "lorem");
			recv(conn, "ipsum");
			send(conn, "dolor");
			recv(conn, "sit");
			send(conn, "amet");
			for (uint32 i = 0; i < 100; i++)
			{
				recv(conn, LongText);
				send(conn, LongText);
			}
			recv(conn, "sit");
			send(conn, "amet");
		}

		void recv(Holder<WebsocketConnection> &conn, const String &msg)
		{
			while (conn->size() == 0)
				threadYield();
			const String line = String(conn->readAll());
			CAGE_TEST(line == msg);
		}

		void send(Holder<WebsocketConnection> &conn, const String &msg)
		{
			while (randomChance() < 0.2)
				threadYield();
			conn->write(msg);
		}
	};
}

void testNetworkWebsocket()
{
	CAGE_TESTCASE("network websocket");

	{
		CAGE_TESTCASE("lorem ipsum");
		LoremIpsumTest test;
	}
}
