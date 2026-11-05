#include "common.h"

#include <cage-core/concurrent.h>
#include <cage-core/core.h>
#include <cage-core/math.h>

Runner::Runner() : last(applicationTime())
{
	const uint32 freq = randomRange(10, 100);
	CAGE_LOG(SeverityEnum::Info, "runner", Stringizer() + "frequency: " + freq + " updates per second");
	period = Time(1'000'000 / freq);
}

void Runner::step()
{
	const Time curr = applicationTime();

	if (curr > last + period * Time(2))
	{
		CAGE_LOG(SeverityEnum::Warning, "runner", "cannot keep up");
		last = curr + period;
		return;
	}

	// sleep = period - elapsed = period - (curr - last)
	const Time slp = period + last - curr;
	if (slp > Time())
		threadSleep(slp);

	last += period;
}
