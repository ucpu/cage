#include <cage-core/core.h>
#include <cage-core/config.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "conn.h"
#include "runner.h"

void runClient()
{
	CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "running in client mode");

	ConfigString address("address");
	ConfigUint32 port("port");
	CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "address: '" + (string)address + "'");
	CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "port: " + (uint32)port);

	Holder<connClass> client = newConn(newUdpConnection(address, port, 0));
	runnerStruct runner;
	while (!client->process())
		runner.step();
}
