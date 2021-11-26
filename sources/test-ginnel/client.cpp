#include <cage-core/core.h>
#include <cage-core/config.h>
#include <cage-core/networkGinnel.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "conn.h"
#include "runner.h"

void runClient()
{
	CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "running in client mode");

	ConfigString address("address");
	ConfigUint32 port("port");
	CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "address: '" + (String)address + "'");
	CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "port: " + (uint32)port);

	Holder<Conn> client = newConn(newGinnelConnection(address, port, 0));
	Runner runner;
	while (!client->process())
		runner.step();
}
