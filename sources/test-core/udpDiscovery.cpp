#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

void testUdpDiscovery()
{
	CAGE_TESTCASE("udp discovery");

	holder<discoveryServerClass> srv = newDiscoveryServer(4243, 1342, 5555);
	holder<discoveryClientClass> clt = newDiscoveryClient(4243, 5555);

	uint32 round = 0;
	while (clt->peersCount() == 0 && round++ < 50)
	{
		srv->message = string() + "Hi " + round;
		srv->update();
		clt->update();
		threadSleep(1000 * 10);
	}

	uint32 cnt = clt->peersCount();
	CAGE_TEST(cnt > 0);
	CAGE_LOG(severityEnum::Info, "test", string() + "got " + cnt + " responses");
	for (uint32 i = 0; i < cnt; i++)
	{
		string msg;
		string addr;
		uint16 port;
		clt->peerData(i, msg, addr, port);
		CAGE_LOG(severityEnum::Info, "test", string() + "address: " + addr + ", port: " + port + ", message: " + msg);
	}
}