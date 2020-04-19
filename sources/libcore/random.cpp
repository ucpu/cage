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
			return RandomGenerator(s);
		}
	}

	RandomGenerator::RandomGenerator()
	{
		privat::generateRandomData((uint8*)s, sizeof(s));
	}

	RandomGenerator::RandomGenerator(uint64 s[2])
	{
		this->s[0] = s[0];
		this->s[1] = s[1];
	}

	RandomGenerator::RandomGenerator(uint64 s1, uint64 s2)
	{
		s[0] = s1;
		s[1] = s2;
	}

	uint64 RandomGenerator::next()
	{
		uint64 x = s[0];
		const uint64 y = s[1];
		s[0] = y;
		x ^= x << 23;
		s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
		return s[1] + y;
	}

#define GCHL_GENERATE(TYPE) TYPE RandomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		if (min == max) \
			return min; \
		CAGE_ASSERT(min < max); \
		uint64 range = max - min; \
		uint64 mod = next() % range; \
		TYPE res = (TYPE)(mod + min); \
		CAGE_ASSERT(res >= min && res < max); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64));
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) TYPE RandomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		CAGE_ASSERT(min <= max); \
		TYPE res = interpolate(min, max, this->randomChance()); \
		return res; \
	}
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, real, rads, degs, float));
#undef GCHL_GENERATE

	double RandomGenerator::randomRange(double min, double max)
	{
		CAGE_ASSERT(min <= max);
		uint64 r = next();
		double f = (double)r / (double)std::numeric_limits<uint64>::max();
		if (f >= 1.0)
			f = 0;
		double res = (max - min) * f + min;
		CAGE_ASSERT(res >= min && res < max);
		return res;
	}

	real RandomGenerator::randomChance()
	{
		uint64 r = next();
		real res = (real)r / (real)std::numeric_limits<uint64>::max();
		if (res >= 1.0)
			res = 0;
		CAGE_ASSERT(res >= 0.f && res < 1.f);
		return res;
	}

	rads RandomGenerator::randomAngle()
	{
		return rads(randomChance() * real::Pi() * real::Pi());
	}

	vec2 RandomGenerator::randomDirection2()
	{
		rads ang = randomAngle();
		return vec2(sin(ang), cos(ang));
	}

	vec3 RandomGenerator::randomDirection3()
	{
		real z = randomRange(-1.f, 1.f);
		vec2 p = randomDirection2() * sqrt(1 - sqr(z));
		vec3 r = vec3(p, z);
		CAGE_ASSERT(abs(lengthSquared(r) - 1) < 1e-5);
		return r;
	}

	quat RandomGenerator::randomDirectionQuat()
	{
		return normalize(quat(randomDirection3(), randomAngle()));
	}

	namespace detail
	{
		RandomGenerator &getApplicationRandomGenerator()
		{
			static RandomGenerator rnd = initializeDefaultGenerator();
			return rnd;
		}
	}

	real randomChance()
	{
		return detail::getApplicationRandomGenerator().randomChance();
	}

	rads randomAngle()
	{
		return detail::getApplicationRandomGenerator().randomAngle();
	}

	vec2 randomDirection2()
	{
		return detail::getApplicationRandomGenerator().randomDirection2();
	}

	vec3 randomDirection3()
	{
		return detail::getApplicationRandomGenerator().randomDirection3();
	}

	quat randomDirectionQuat()
	{
		return detail::getApplicationRandomGenerator().randomDirectionQuat();
	}

#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max) { return detail::getApplicationRandomGenerator().randomRange(min, max); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, degs, float, double));
#undef GCHL_GENERATE
}
