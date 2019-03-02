
#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/random.h>
#include <cage-core/identifier.h>

namespace cage
{
	randomGenerator::randomGenerator()
	{
		privat::generateRandomData((uint8*)s, sizeof(s));
	}

	randomGenerator::randomGenerator(uint64 s[2])
	{
		this->s[0] = s[0];
		this->s[1] = s[1];
	}

	randomGenerator::randomGenerator(uint64 s1, uint64 s2)
	{
		s[0] = s1;
		s[1] = s2;
	}

	uint64 randomGenerator::next()
	{
		uint64 x = s[0];
		const uint64 y = s[1];
		s[0] = y;
		x ^= x << 23;
		s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
		return s[1] + y;
	}

#define GCHL_GENERATE(TYPE) TYPE randomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		if (min == max) \
			return min; \
		CAGE_ASSERT_RUNTIME(min < max, min, max); \
		uint64 range = max - min; \
		uint64 mod = next() % range; \
		TYPE res = (TYPE)(mod + min); \
		CAGE_ASSERT_RUNTIME(res >= min && res < max, res, min, max); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64));
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) TYPE randomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		CAGE_ASSERT_RUNTIME(min <= max, min, max); \
		TYPE res = interpolate(min, max, this->randomChance()); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, real, rads, float));
#undef GCHL_GENERATE

	double randomGenerator::randomRange(double min, double max)
	{
		CAGE_ASSERT_RUNTIME(min <= max, min, max);
		uint64 r = next();
		double f = (double)r / (double)detail::numeric_limits<uint64>::max();
		if (f >= 1.0)
			f = 0;
		double res = (max - min) * f + min;
		CAGE_ASSERT_RUNTIME(res >= min && res < max, res, min, max);
		return res;
	}

	real randomGenerator::randomChance()
	{
		uint64 r = next();
		real res = (real)r / (real)detail::numeric_limits<uint64>::max();
		if (res >= 1.0)
			res = 0;
		CAGE_ASSERT_RUNTIME(res >= 0.f && res < 1.f, res);
		return res;
	}

	rads randomGenerator::randomAngle()
	{
		return rads(randomChance() * real::TwoPi);
	}

	vec2 randomGenerator::randomDirection2()
	{
		rads ang = randomAngle();
		return vec2(sin(ang), cos(ang));
	}

	vec3 randomGenerator::randomDirection3()
	{
		real z = randomRange(-1.f, 1.f);
		vec2 p = randomDirection2() * sqrt(1 - sqr(z));
		vec3 r = vec3(p, z);
		CAGE_ASSERT_RUNTIME(abs(r.squaredLength() - 1) < 1e-5);
		return r;
	}

	quat randomGenerator::randomDirectionQuat()
	{
		return normalize(quat(randomDirection3(), randomAngle()));
	}

	randomGenerator &currentRandomGenerator()
	{
		static randomGenerator rnd;
		return rnd;
	}

	real randomChance()
	{
		return currentRandomGenerator().randomChance();
	}

	rads randomAngle()
	{
		return currentRandomGenerator().randomAngle();
	}

	vec2 randomDirection2()
	{
		return currentRandomGenerator().randomDirection2();
	}

	vec3 randomDirection3()
	{
		return currentRandomGenerator().randomDirection3();
	}

	quat randomDirectionQuat()
	{
		return currentRandomGenerator().randomDirectionQuat();
	}

#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max) { return currentRandomGenerator().randomRange(min, max); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, float, double));
#undef GCHL_GENERATE
}
