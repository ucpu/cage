#include "main.h"

#include <cage-core/guid.h>
#include <cage-core/math.h>
#include <cage-core/random.h>

void test(const Vec2 &a, const Vec2 &b);
void test(const Vec3 &a, const Vec3 &b);
void test(const Vec4 &a, const Vec4 &b);

void testRandom()
{
	CAGE_TESTCASE("random");

	{
		CAGE_TESTCASE("random chances (0 - 1)");
		for (uint32 i = 0; i < 5; i++)
		{
			const Real r = randomChance();
			CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + r);
			CAGE_TEST(r >= 0 && r < 1);
		}
	}

	{
		CAGE_TESTCASE("random generator (random seed)");
		for (uint32 i = 0; i < 2; i++)
		{
			RandomGenerator g;
			CAGE_LOG(SeverityEnum::Info, "generator seed", Stringizer() + g.s[0] + " " + g.s[1]);
			for (uint32 i = 0; i < 3; i++)
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "random sequence", Stringizer() + g.next());
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
			const Vec2i v = g.randomRange2i(10, 1000);
			CAGE_TEST(v == Vec2i(730, 955));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const Vec3i v = g.randomRange3i(10, 1000);
			CAGE_TEST(v == Vec3i(730, 955, 866));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const Vec4i v = g.randomRange4i(10, 1000);
			CAGE_TEST(v == Vec4i(730, 955, 866, 863));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const Vec2 v = g.randomRange2(10, 1000);
			test(v, Vec2(10.4162464, 931.966248));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const Vec3 v = g.randomRange3(10, 1000);
			test(v, Vec3(10.4162464, 931.966248, 645.401428));
		}
		{
			RandomGenerator g(1346519564496, 42245614964156);
			const Vec4 v = g.randomRange4(10, 1000);
			test(v, Vec4(10.4162464, 931.966248, 645.401428, 848.857239));
		}
	}

	{
		CAGE_TESTCASE("random distribution");
		RandomGenerator g1(1346519564496, 42245614964156);
		constexpr uint32 count = 5;
		uint32 buckets[count] = {};
		for (uint32 i = 0; i < 1000; i++)
		{
			const uint32 r = g1.randomRange(0u, count);
			CAGE_TEST(r < count);
			buckets[r]++;
		}
		for (uint32 v : buckets)
			CAGE_LOG(SeverityEnum::Info, "random-distribution", Stringizer() + v);
	}

	{
		CAGE_TESTCASE("guid");
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<32>(true));
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<32>(true));
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<32>(true));
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "random identifier", Guid<8>(true));
	}
}
