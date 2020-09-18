#include "main.h"

#include <cage-core/math.h>
#include <cage-core/guid.h>
#include <cage-core/random.h>

void test(const vec2 &a, const vec2 &b);
void test(const vec3 &a, const vec3 &b);
void test(const vec4 &a, const vec4 &b);

void testRandom()
{
	CAGE_TESTCASE("random");

	{
		CAGE_TESTCASE("random chances (0 - 1)");
		for (uint32 i = 0; i < 5; i++)
		{
			const real r = randomChance();
			CAGE_LOG(SeverityEnum::Info, "test", stringizer() + r);
			CAGE_TEST(r >= 0 && r < 1);
		}
	}

	{
		CAGE_TESTCASE("random generator (random seed)");
		for (uint32 i = 0; i < 2; i++)
		{
			RandomGenerator g;
			CAGE_LOG(SeverityEnum::Info, "generator seed", stringizer() + g.s[0] + " " + g.s[1]);
			for (uint32 i = 0; i < 3; i++)
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "random sequence", stringizer() + g.next());
		}
	}

	{
		CAGE_TESTCASE("random generators (fixed seed)");
		RandomGenerator g1(1346519564496, 42245614964156);
		RandomGenerator g2(1346519564496, 42245614964156);
		for (uint32 i = 0; i < 3; i++)
		{
			CAGE_TEST(g1.next() == g2.next());
		}
	}

	{
		CAGE_TESTCASE("verify that all compilers produce vector values in the same order");
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const ivec2 v = g.randomRange2i(10, 1000);
			CAGE_TEST(v == ivec2(721, 670));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const ivec3 v = g.randomRange3i(10, 1000);
			CAGE_TEST(v == ivec3(721, 670, 70));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const ivec4 v = g.randomRange4i(10, 1000);
			CAGE_TEST(v == ivec4(721, 670, 70, 385));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const vec2 v = g.randomRange2(10, 1000);
			test(v, vec2(616.199036, 286.644745));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const vec3 v = g.randomRange3(10, 1000);
			test(v, vec3(616.199036, 286.644745, 843.249634));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const vec4 v = g.randomRange4(10, 1000);
			test(v, vec4(616.199036, 286.644745, 843.249634, 29.7623386));
		}
	}

	{
		CAGE_TESTCASE("guid");
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<32>(true));
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<32>(true));
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<32>(true));
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<8>(true));
	}
}
