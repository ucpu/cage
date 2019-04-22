#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "conn.h"

void runClient()
{
	CAGE_LOG(severityEnum::Info, "config", string() + "running in client mode");

	configString address("address");
	configUint32 port("port");
	CAGE_LOG(severityEnum::Info, "config", string() + "address: '" + address + "'");
	CAGE_LOG(severityEnum::Info, "config", string() + "port: " + (uint32)port);

	holder<connClass> client = newConn(newUdpConnection(address, port, 0));
	uint64 time = getApplicationTime();
	while (!client->process())
	{
		uint64 t = getApplicationTime();
		sint64 s = numeric_cast<sint64>(time + timeStep) - numeric_cast<sint64>(t);
		if (s > 0)
			threadSleep(s);
		else
			CAGE_LOG(severityEnum::Warning, "client", "cannot keep up");
		time += timeStep;
	}
}
