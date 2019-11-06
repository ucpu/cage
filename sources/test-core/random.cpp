#include "main.h"

#include <cage-core/math.h>
#include <cage-core/identifier.h>
#include <cage-core/random.h>

void testRandom()
{
	CAGE_TESTCASE("random");

	{
		CAGE_TESTCASE("random numbers");
		for (uint32 i = 0; i < 5; i++)
		{
			real r = randomChance();
			CAGE_TEST(r >= 0 && r < 1, r);
		}
	}

	{
		CAGE_TESTCASE("random generator (random seed)");
		for (uint32 i = 0; i < 2; i++)
		{
			randomGenerator r;
			CAGE_LOG(severityEnum::Info, "generator seed", stringizer() + r.s[0] + " " + r.s[1]);
			for (uint32 i = 0; i < 3; i++)
				CAGE_LOG_CONTINUE(severityEnum::Info, "random sequence", string(r.next()));
		}
	}

	{
		CAGE_TESTCASE("random generator (fixed seed)");
		for (uint32 i = 0; i < 2; i++)
		{
			randomGenerator r(13, 42);
			CAGE_LOG(severityEnum::Info, "generator seed", stringizer() + r.s[0] + " " + r.s[1]);
			for (uint32 i = 0; i < 3; i++)
				CAGE_LOG_CONTINUE(severityEnum::Info, "random sequence", string(r.next()));
		}
	}

	{
		CAGE_TESTCASE("identifier");
		CAGE_LOG_CONTINUE(severityEnum::Info, "random identifier", identifier<32>(true));
		CAGE_LOG_CONTINUE(severityEnum::Info, "random identifier", identifier<32>(true));
		CAGE_LOG_CONTINUE(severityEnum::Info, "random identifier", identifier<32>(true));
		CAGE_LOG_CONTINUE(severityEnum::Info, "random identifier", identifier<8>(true));
	}
}
