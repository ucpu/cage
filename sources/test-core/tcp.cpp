#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

void senderThread()
{
	holder<tcpConnection> sender = newTcpConnection("localhost", 4241);
	sender->writeLine("ahoj");
	sender->writeLine("nazdar");
	sender->writeLine("cau");
}

void testTcp()
{
	CAGE_TESTCASE("tcp");

	holder<tcpServer> server = newTcpServer(4241);
	holder<thread> thr = newThread(delegate<void()>().bind<&senderThread>(), "tcp test sender");
	holder<tcpConnection> receiver;
	while (!receiver)
	{
		threadSleep(1000);
		receiver = server->accept();
	}
	string line;
	while (!receiver->readLine(line))
		threadSleep(1000);
	CAGE_TEST(line == "ahoj");
	while (!receiver->readLine(line))
		threadSleep(1000);
	CAGE_TEST(line == "nazdar");
	while (!receiver->readLine(line))
		threadSleep(1000);
	CAGE_TEST(line == "cau");
}
