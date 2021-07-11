#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "runner.h"

Runner::Runner() : last(applicationTime())
{
	const uint32 freq = randomRange(10, 100);
	CAGE_LOG(SeverityEnum::Info, "runner", stringizer() + "frequency: " + freq + " updates per second");
	period = 1000000 / freq;
}

void Runner::step()
{
	const uint64 curr = applicationTime();

	if (curr > last + period * 2)
	{
		CAGE_LOG(SeverityEnum::Warning, "runner", "cannot keep up");
		last = curr + period;
		return;
	}

	// sleep = period - elapsed = period - (curr - last)
	const sint64 slp = period + last - curr;
	if (slp > 0)
		threadSleep(slp);

	last += period;
}
