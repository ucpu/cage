#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "runner.h"

runnerStruct::runnerStruct() : time(getApplicationTime()), timeStep(1000000 / (randomRange(20, 40)))
{}

void runnerStruct::step()
{
	uint64 t = getApplicationTime();
	sint64 s = time + timeStep - t;
	if (s > 0)
		threadSleep(s);
	else
		CAGE_LOG(severityEnum::Warning, "runner", "cannot keep up");
	time += timeStep;
}