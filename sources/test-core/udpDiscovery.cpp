#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

void testUdpDiscovery()
{
	CAGE_TESTCASE("udp discovery");

	Holder<DiscoveryServer> srv = newDiscoveryServer(4243, 1342, 5555);
	Holder<DiscoveryClient> clt = newDiscoveryClient(4243, 5555);

	uint32 round = 0;
	while (clt->peersCount() == 0 && round++ < 50)
	{
		srv->message = stringizer() + "Hi " + round;
		srv->update();
		clt->update();
		threadSleep(1000 * 10);
	}

	uint32 cnt = clt->peersCount();
	CAGE_LOG(SeverityEnum::Info, "test", stringizer() + "got " + cnt + " responses");
	if (cnt == 0)
	{
		CAGE_LOG(SeverityEnum::Error, "test", "udp discovery failed!");
		CAGE_LOG(SeverityEnum::Note, "test", "the network may block broadcast messages");
		CAGE_LOG(SeverityEnum::Note, "test", "or a firewall may block the ports");
	}
	else
	{
		for (const auto &r : clt->peers())
		{
			CAGE_LOG(SeverityEnum::Info, "test", stringizer() + "address: " + r.address + ", message: " + r.message);
			CAGE_TEST(r.port == 1342);
		}
	}
}
