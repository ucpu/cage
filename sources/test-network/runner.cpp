#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "runner.h"

Runner::Runner() : time(getApplicationTime()), timeStep(1000000 / (randomRange(30, 60)))
{}

void Runner::step()
{
	const uint64 t = getApplicationTime();
	sint64 s = time + timeStep - t;
	if (s >= 0)
	{
		threadSleep(s);
		time += timeStep;
	}
	else
	{
		CAGE_LOG(SeverityEnum::Warning, "runner", "cannot keep up");
		time = t + timeStep;
	}
}
