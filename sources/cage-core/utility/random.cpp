//#include <random>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/utility/random.h>
#include <cage-core/utility/identifier.h>

namespace cage
{
	randomGenerator::randomGenerator()
	{
		privat::generateRandomData((uint8*)s, sizeof(s));
		/*
		std::random_device rd;
		this->s[0] = (((uint64)rd()) << 32) + rd();
		this->s[1] = (((uint64)rd()) << 32) + rd();
		*/
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

	real randomGenerator::random()
	{
		uint64 r = next();
		if (r == detail::numeric_limits<uint64>::max())
			return 0;
		return (real)r / (real)detail::numeric_limits<uint64>::max();
	}

	sint32 randomGenerator::random(sint32 min, sint32 max)
	{
		if (min == max)
			return min;
		CAGE_ASSERT_RUNTIME(min < max, min, max);
		uint64 range = max - min;
		sint64 mod = next() % range;
		sint32 res = numeric_cast<sint32>(mod + min);
		CAGE_ASSERT_RUNTIME(res >= min && res < max, res, min, max);
		return res;
	}

	real randomGenerator::random(real min, real max)
	{
		return interpolate(min, max, random());
	}

	rads randomGenerator::random(rads min, rads max)
	{
		return interpolate(min, max, random());
	}

	rads randomGenerator::randomAngle()
	{
		return rads(random() * real::TwoPi);
	}

	vec2 randomGenerator::randomDirection2()
	{
		rads ang = randomAngle();
		return vec2(sin(ang), cos(ang));
	}

	vec3 randomGenerator::randomDirection3()
	{
		real u = random();
		real v = random();
		rads theta = rads(real::TwoPi * u);
		rads phi = aCos(2 * v - 1);
		return vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi)).normalize();
	}

	quat randomGenerator::randomDirectionQuat()
	{
		return normalize(quat(randomDirection3(), randomAngle()));
	}
}