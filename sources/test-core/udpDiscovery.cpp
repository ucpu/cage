#include "main.h"
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

void testUdpDiscovery()
{
	CAGE_TESTCASE("udp discovery");

	holder<discoveryServer> srv = newDiscoveryServer(4243, 1342, 5555);
	holder<discoveryClient> clt = newDiscoveryClient(4243, 5555);

	uint32 round = 0;
	while (clt->peersCount() == 0 && round++ < 50)
	{
		srv->message = string() + "Hi " + round;
		srv->update();
		clt->update();
		threadSleep(1000 * 10);
	}

	uint32 cnt = clt->peersCount();
	CAGE_LOG(severityEnum::Info, "test", string() + "got " + cnt + " responses");
	CAGE_TEST(cnt > 0);
	for (const auto &r : clt->peers())
	{
		CAGE_LOG(severityEnum::Info, "test", string() + "address: " + r.address + ", message: " + r.message);
		CAGE_TEST(r.port == 1342);
	}
}
