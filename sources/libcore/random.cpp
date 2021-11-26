#include <cage-core/math.h>
#include <cage-core/random.h>
#include <cage-core/guid.h>
#include <cage-core/config.h>
#include <cage-core/macros.h>

namespace cage
{
	namespace
	{
		ConfigUint64 confDefault1("cage/random/seed1", 0);
		ConfigUint64 confDefault2("cage/random/seed2", 0);

		RandomGenerator initializeDefaultGenerator()
		{
			uint64 s[2];
			s[0] = confDefault1;
			s[1] = confDefault2;
			if (s[0] == 0 && s[1] == 0)
				privat::generateRandomData((uint8*)s, sizeof(s));
			CAGE_LOG(SeverityEnum::Info, "random", Stringizer() + "initializing default random generator: " + s[0] + ", " + s[1]);
			return RandomGenerator(s[0], s[1]);
		}
	}

	RandomGenerator::RandomGenerator()
	{
		privat::generateRandomData((uint8*)s, sizeof(s));
	}

	RandomGenerator::RandomGenerator(uint64 s1, uint64 s2) : s{s1, s2}
	{}

	uint64 RandomGenerator::next()
	{
		uint64 x = s[0];
		const uint64 y = s[1];
		s[0] = y;
		x ^= x << 23;
		s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
		return s[1] + y;
	}

	Real RandomGenerator::randomChance()
	{
		const uint64 r = next();
		Real res = (Real)r / (Real)std::numeric_limits<uint64>::max();
		if (res >= 1.0)
			res = 0;
		CAGE_ASSERT(res >= 0.f && res < 1.f);
		return res;
	}

#define GCHL_GENERATE(TYPE) TYPE RandomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		if (min == max) \
			return min; \
		CAGE_ASSERT(min < max); \
		const uint64 range = max - min; \
		const uint64 mod = next() % range; \
		TYPE res = (TYPE)(mod + min); \
		CAGE_ASSERT(res >= min && res < max); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64));
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) TYPE RandomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		CAGE_ASSERT(min <= max); \
		const Real c = randomChance(); \
		const TYPE res = interpolate(min, max, c); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, Real, Rads, Degs, float));
#undef GCHL_GENERATE

	double RandomGenerator::randomRange(double min, double max)
	{
		CAGE_ASSERT(min <= max);
		const uint64 r = next();
		double f = (double)r / (double)std::numeric_limits<uint64>::max();
		if (f >= 1.0)
			f = 0;
		const double res = (max - min) * f + min;
		CAGE_ASSERT(res >= min && res < max);
		return res;
	}

	Rads RandomGenerator::randomAngle()
	{
		return Rads(randomChance() * Real::Pi() * 2);
	}

	Vec2 RandomGenerator::randomChance2()
	{
		const Real r1 = randomChance();
		const Real r2 = randomChance();
		return Vec2(r1, r2);
	}

	Vec2 RandomGenerator::randomRange2(Real a, Real b)
	{
		const Real r1 = randomRange(a, b);
		const Real r2 = randomRange(a, b);
		return Vec2(r1, r2);
	}

	Vec2 RandomGenerator::randomDirection2()
	{
		const Rads ang = randomAngle();
		return Vec2(sin(ang), cos(ang));
	}

	Vec2i RandomGenerator::randomRange2i(sint32 a, sint32 b)
	{
		const sint32 r1 = randomRange(a, b);
		const sint32 r2 = randomRange(a, b);
		return Vec2i(r1, r2);
	}

	Vec3 RandomGenerator::randomChance3()
	{
		const Real r1 = randomChance();
		const Real r2 = randomChance();
		const Real r3 = randomChance();
		return Vec3(r1, r2, r3);
	}

	Vec3 RandomGenerator::randomRange3(Real a, Real b)
	{
		const Real r1 = randomRange(a, b);
		const Real r2 = randomRange(a, b);
		const Real r3 = randomRange(a, b);
		return Vec3(r1, r2, r3);
	}

	Vec3 RandomGenerator::randomDirection3()
	{
		const Real z = randomRange(-1.f, 1.f);
		const Vec2 p = randomDirection2() * sqrt(1 - sqr(z));
		const Vec3 r = Vec3(p, z);
		CAGE_ASSERT(abs(lengthSquared(r) - 1) < 1e-5);
		return r;
	}

	Vec3i RandomGenerator::randomRange3i(sint32 a, sint32 b)
	{
		const sint32 r1 = randomRange(a, b);
		const sint32 r2 = randomRange(a, b);
		const sint32 r3 = randomRange(a, b);
		return Vec3i(r1, r2, r3);
	}

	Vec4 RandomGenerator::randomChance4()
	{
		const Real r1 = randomChance();
		const Real r2 = randomChance();
		const Real r3 = randomChance();
		const Real r4 = randomChance();
		return Vec4(r1, r2, r3, r4);
	}

	Vec4 RandomGenerator::randomRange4(Real a, Real b)
	{
		const Real r1 = randomRange(a, b);
		const Real r2 = randomRange(a, b);
		const Real r3 = randomRange(a, b);
		const Real r4 = randomRange(a, b);
		return Vec4(r1, r2, r3, r4);
	}

	Quat RandomGenerator::randomDirectionQuat()
	{
		const Vec3 d = randomDirection3();
		const Rads a = randomAngle();
		return normalize(Quat(d, a));
	}

	Vec4i RandomGenerator::randomRange4i(sint32 a, sint32 b)
	{
		const sint32 r1 = randomRange(a, b);
		const sint32 r2 = randomRange(a, b);
		const sint32 r3 = randomRange(a, b);
		const sint32 r4 = randomRange(a, b);
		return Vec4i(r1, r2, r3, r4);
	}

	namespace detail
	{
		RandomGenerator &globalRandomGenerator()
		{
			static RandomGenerator rnd = initializeDefaultGenerator();
			return rnd;
		}
	}

	Real randomChance()
	{
		return detail::globalRandomGenerator().randomChance();
	}

#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max) { return detail::globalRandomGenerator().randomRange(min, max); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, Real, Rads, Degs, float, double));
#undef GCHL_GENERATE

	Rads randomAngle()
	{
		return detail::globalRandomGenerator().randomAngle();
	}

	Vec2 randomChance2()
	{
		return detail::globalRandomGenerator().randomChance2();
	}

	Vec2 randomRange2(Real a, Real b)
	{
		return detail::globalRandomGenerator().randomRange2(a, b);
	}

	Vec2 randomDirection2()
	{
		return detail::globalRandomGenerator().randomDirection2();
	}

	Vec2i randomRange2i(sint32 a, sint32 b)
	{
		return detail::globalRandomGenerator().randomRange2i(a, b);
	}

	Vec3 randomChance3()
	{
		return detail::globalRandomGenerator().randomChance3();
	}

	Vec3 randomRange3(Real a, Real b)
	{
		return detail::globalRandomGenerator().randomRange3(a, b);
	}

	Vec3 randomDirection3()
	{
		return detail::globalRandomGenerator().randomDirection3();
	}

	Vec3i randomRange3i(sint32 a, sint32 b)
	{
		return detail::globalRandomGenerator().randomRange3i(a, b);
	}

	Vec4 randomChance4()
	{
		return detail::globalRandomGenerator().randomChance4();
	}

	Vec4 randomRange4(Real a, Real b)
	{
		return detail::globalRandomGenerator().randomRange4(a, b);
	}

	Quat randomDirectionQuat()
	{
		return detail::globalRandomGenerator().randomDirectionQuat();
	}

	Vec4i randomRange4i(sint32 a, sint32 b)
	{
		return detail::globalRandomGenerator().randomRange4i(a, b);
	}
}
