#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

void senderThread()
{
	holder<tcpConnectionClass> sender = newTcpConnection("localhost", 4241);
	sender->writeLine("ahoj");
	sender->writeLine("nazdar");
	sender->writeLine("cau");
}

void testTcp()
{
	CAGE_TESTCASE("tcp");

	holder<tcpServerClass> server = newTcpServer(4241);
	holder<threadClass> thr = newThread(delegate<void()>().bind<&senderThread>(), "tcp test sender");
	holder<tcpConnectionClass> receiver;
	while (!receiver)
	{
		threadSleep(1000);
		receiver = server->accept();
	}
	string line;
	while (!receiver->availableLine())
		threadSleep(1000);
	CAGE_TEST(receiver->readLine(line));
	CAGE_TEST(line == "ahoj");
	while (!receiver->availableLine())
		threadSleep(1000);
	CAGE_TEST(receiver->readLine(line));
	CAGE_TEST(line == "nazdar");
	while (!receiver->availableLine())
		threadSleep(1000);
	CAGE_TEST(receiver->readLine(line));
	CAGE_TEST(line == "cau");
}