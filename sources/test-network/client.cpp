#include <cage-core/core.h>
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "conn.h"
#include "runner.h"

void runClient()
{
	CAGE_LOG(severityEnum::Info, "config", string() + "running in client mode");

	//configSetSint32("cage-core.udp.logLevel", 10);

	configString address("address");
	configUint32 port("port");
	CAGE_LOG(severityEnum::Info, "config", string() + "address: '" + address + "'");
	CAGE_LOG(severityEnum::Info, "config", string() + "port: " + (uint32)port);

	holder<connClass> client = newConn(newUdpConnection(address, port, 0));
	runnerStruct runner;
	while (!client->process())
		runner.step();
}
