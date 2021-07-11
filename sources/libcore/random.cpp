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
			CAGE_LOG(SeverityEnum::Info, "random", stringizer() + "initializing default random generator: " + s[0] + ", " + s[1]);
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

	real RandomGenerator::randomChance()
	{
		const uint64 r = next();
		real res = (real)r / (real)std::numeric_limits<uint64>::max();
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
		const real c = randomChance(); \
		const TYPE res = interpolate(min, max, c); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, real, rads, degs, float));
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

	rads RandomGenerator::randomAngle()
	{
		return rads(randomChance() * real::Pi() * 2);
	}

	vec2 RandomGenerator::randomChance2()
	{
		const real r1 = randomChance();
		const real r2 = randomChance();
		return vec2(r1, r2);
	}

	vec2 RandomGenerator::randomRange2(real a, real b)
	{
		const real r1 = randomRange(a, b);
		const real r2 = randomRange(a, b);
		return vec2(r1, r2);
	}

	vec2 RandomGenerator::randomDirection2()
	{
		const rads ang = randomAngle();
		return vec2(sin(ang), cos(ang));
	}

	ivec2 RandomGenerator::randomRange2i(sint32 a, sint32 b)
	{
		const sint32 r1 = randomRange(a, b);
		const sint32 r2 = randomRange(a, b);
		return ivec2(r1, r2);
	}

	vec3 RandomGenerator::randomChance3()
	{
		const real r1 = randomChance();
		const real r2 = randomChance();
		const real r3 = randomChance();
		return vec3(r1, r2, r3);
	}

	vec3 RandomGenerator::randomRange3(real a, real b)
	{
		const real r1 = randomRange(a, b);
		const real r2 = randomRange(a, b);
		const real r3 = randomRange(a, b);
		return vec3(r1, r2, r3);
	}

	vec3 RandomGenerator::randomDirection3()
	{
		const real z = randomRange(-1.f, 1.f);
		const vec2 p = randomDirection2() * sqrt(1 - sqr(z));
		const vec3 r = vec3(p, z);
		CAGE_ASSERT(abs(lengthSquared(r) - 1) < 1e-5);
		return r;
	}

	ivec3 RandomGenerator::randomRange3i(sint32 a, sint32 b)
	{
		const sint32 r1 = randomRange(a, b);
		const sint32 r2 = randomRange(a, b);
		const sint32 r3 = randomRange(a, b);
		return ivec3(r1, r2, r3);
	}

	vec4 RandomGenerator::randomChance4()
	{
		const real r1 = randomChance();
		const real r2 = randomChance();
		const real r3 = randomChance();
		const real r4 = randomChance();
		return vec4(r1, r2, r3, r4);
	}

	vec4 RandomGenerator::randomRange4(real a, real b)
	{
		const real r1 = randomRange(a, b);
		const real r2 = randomRange(a, b);
		const real r3 = randomRange(a, b);
		const real r4 = randomRange(a, b);
		return vec4(r1, r2, r3, r4);
	}

	quat RandomGenerator::randomDirectionQuat()
	{
		const vec3 d = randomDirection3();
		const rads a = randomAngle();
		return normalize(quat(d, a));
	}

	ivec4 RandomGenerator::randomRange4i(sint32 a, sint32 b)
	{
		const sint32 r1 = randomRange(a, b);
		const sint32 r2 = randomRange(a, b);
		const sint32 r3 = randomRange(a, b);
		const sint32 r4 = randomRange(a, b);
		return ivec4(r1, r2, r3, r4);
	}

	namespace detail
	{
		RandomGenerator &globalRandomGenerator()
		{
			static RandomGenerator rnd = initializeDefaultGenerator();
			return rnd;
		}
	}

	real randomChance()
	{
		return detail::globalRandomGenerator().randomChance();
	}

#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max) { return detail::globalRandomGenerator().randomRange(min, max); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, degs, float, double));
#undef GCHL_GENERATE

	rads randomAngle()
	{
		return detail::globalRandomGenerator().randomAngle();
	}

	vec2 randomChance2()
	{
		return detail::globalRandomGenerator().randomChance2();
	}

	vec2 randomRange2(real a, real b)
	{
		return detail::globalRandomGenerator().randomRange2(a, b);
	}

	vec2 randomDirection2()
	{
		return detail::globalRandomGenerator().randomDirection2();
	}

	ivec2 randomRange2i(sint32 a, sint32 b)
	{
		return detail::globalRandomGenerator().randomRange2i(a, b);
	}

	vec3 randomChance3()
	{
		return detail::globalRandomGenerator().randomChance3();
	}

	vec3 randomRange3(real a, real b)
	{
		return detail::globalRandomGenerator().randomRange3(a, b);
	}

	vec3 randomDirection3()
	{
		return detail::globalRandomGenerator().randomDirection3();
	}

	ivec3 randomRange3i(sint32 a, sint32 b)
	{
		return detail::globalRandomGenerator().randomRange3i(a, b);
	}

	vec4 randomChance4()
	{
		return detail::globalRandomGenerator().randomChance4();
	}

	vec4 randomRange4(real a, real b)
	{
		return detail::globalRandomGenerator().randomRange4(a, b);
	}

	quat randomDirectionQuat()
	{
		return detail::globalRandomGenerator().randomDirectionQuat();
	}

	ivec4 randomRange4i(sint32 a, sint32 b)
	{
		return detail::globalRandomGenerator().randomRange4i(a, b);
	}
}
