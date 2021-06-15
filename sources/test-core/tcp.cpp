#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/math.h> // randomChance

namespace
{
	void recv(Holder<TcpConnection> &conn, const string &msg)
	{
		while (randomChance() < 0.2)
			threadYield();
		string line = conn->readLine();
		CAGE_TEST(line == msg);
	}

	void send(Holder<TcpConnection> &conn, const string &msg)
	{
		while (randomChance() < 0.2)
			threadYield();
		conn->writeLine(msg);
	}

	void clientThread()
	{
		Holder<TcpConnection> conn = newTcpConnection("localhost", 4241);
		send(conn, "lorem");
		recv(conn, "ipsum");
		send(conn, "dolor");
		recv(conn, "sit");
		send(conn, "amet");
	}
}

void testTcp()
{
	CAGE_TESTCASE("tcp");
	Holder<TcpServer> server = newTcpServer(4241);
	Holder<Thread> thr = newThread(Delegate<void()>().bind<&clientThread>(), "tcp client");
	Holder<TcpConnection> conn;
	while (!conn)
	{
		threadYield();
		conn = server->accept();
	}
	recv(conn, "lorem");
	send(conn, "ipsum");
	recv(conn, "dolor");
	send(conn, "sit");
	recv(conn, "amet");
	CAGE_TEST_THROWN(conn->readLine()); // connection broken
}
