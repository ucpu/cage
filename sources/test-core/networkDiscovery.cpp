#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/networkDiscovery.h>

void testNetworkDiscovery()
{
	CAGE_TESTCASE("network discovery");

	Holder<DiscoveryServer> srv = newDiscoveryServer(4743, 1342, 5555);
	Holder<DiscoveryClient> clt = newDiscoveryClient(4743, 5555);

	uint32 round = 0;
	while (clt->peers().empty() && round++ < 50)
	{
		srv->message = Stringizer() + "Hi " + round;
		srv->update();
		clt->update();
		threadSleep(1000 * 10);
	}

	const auto peers = clt->peers();
	CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "got " + peers->size() + " responses");
	if (peers.empty())
	{
		CAGE_LOG(SeverityEnum::Error, "test", "udp discovery failed!");
		CAGE_LOG(SeverityEnum::Note, "test", "the network may block broadcast messages");
		CAGE_LOG(SeverityEnum::Note, "test", "or a firewall may block the ports");
	}
	else
	{
		for (const auto &r : peers)
		{
			CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "address: " + r.address + ", message: " + r.message);
			CAGE_TEST(r.port == 1342);
		}
	}
}
