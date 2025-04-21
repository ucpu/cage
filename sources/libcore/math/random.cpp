#include <cage-core/config.h>
#include <cage-core/guid.h>
#include <cage-core/macros.h>
#include <cage-core/math.h>
#include <cage-core/random.h>

namespace cage
{
	RandomGenerator::RandomGenerator()
	{
		privat::generateRandomData((uint8 *)s, sizeof(s));
	}

	RandomGenerator::RandomGenerator(uint64 s1, uint64 s2) : s{ s1, s2 }
	{
		CAGE_ASSERT(s1 != 0 && s2 != 0);
	}

	namespace
	{
		CAGE_FORCE_INLINE uint64 rotl(const uint64 x, int k)
		{
			return (x << k) | (x >> (64 - k));
		}
	}

	uint64 RandomGenerator::next()
	{
		// xoroshiro128**
		// https://prng.di.unimi.it/xoroshiro128starstar.c
		const uint64 s0 = s[0];
		uint64 s1 = s[1];
		const uint64 result = rotl(s0 * 5, 7) * 9;
		s1 ^= s0;
		s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16);
		s[1] = rotl(s1, 37);
		return result;
	}

	Real RandomGenerator::randomChance()
	{
		return randomChanceDouble();
	}

	double RandomGenerator::randomChanceDouble()
	{
		const uint64 r = next();
		const double res = (r >> 11) * 0x1.0p-53;
		CAGE_ASSERT(res >= 0.0 && res < 1.0);
		return res;
	}

#define GCHL_GENERATE(TYPE) \
	TYPE RandomGenerator::randomRange(TYPE min, TYPE max) \
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
	CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64));
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	TYPE RandomGenerator::randomRange(TYPE min, TYPE max) \
	{ \
		CAGE_ASSERT(min <= max); \
		const Real c = randomChance(); \
		const TYPE res = interpolate(min, max, c); \
		return res; \
	}
	CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, Real, Rads, Degs, float));
#undef GCHL_GENERATE

	double RandomGenerator::randomRange(double min, double max)
	{
		CAGE_ASSERT(min <= max);
		const double c = randomChanceDouble();
		const double res = interpolate(min, max, c);
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
		RandomGenerator &randomGenerator()
		{
			thread_local RandomGenerator rnd;
			return rnd;
		}
	}

	Real randomChance()
	{
		return detail::randomGenerator().randomChance();
	}

	double randomChanceDouble()
	{
		return detail::randomGenerator().randomChanceDouble();
	}

#define GCHL_GENERATE(TYPE) \
	TYPE randomRange(TYPE min, TYPE max) \
	{ \
		return detail::randomGenerator().randomRange(min, max); \
	}
	CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, Real, Rads, Degs, float, double));
#undef GCHL_GENERATE

	Rads randomAngle()
	{
		return detail::randomGenerator().randomAngle();
	}

	Vec2 randomChance2()
	{
		return detail::randomGenerator().randomChance2();
	}

	Vec2 randomRange2(Real a, Real b)
	{
		return detail::randomGenerator().randomRange2(a, b);
	}

	Vec2 randomDirection2()
	{
		return detail::randomGenerator().randomDirection2();
	}

	Vec2i randomRange2i(sint32 a, sint32 b)
	{
		return detail::randomGenerator().randomRange2i(a, b);
	}

	Vec3 randomChance3()
	{
		return detail::randomGenerator().randomChance3();
	}

	Vec3 randomRange3(Real a, Real b)
	{
		return detail::randomGenerator().randomRange3(a, b);
	}

	Vec3 randomDirection3()
	{
		return detail::randomGenerator().randomDirection3();
	}

	Vec3i randomRange3i(sint32 a, sint32 b)
	{
		return detail::randomGenerator().randomRange3i(a, b);
	}

	Vec4 randomChance4()
	{
		return detail::randomGenerator().randomChance4();
	}

	Vec4 randomRange4(Real a, Real b)
	{
		return detail::randomGenerator().randomRange4(a, b);
	}

	Quat randomDirectionQuat()
	{
		return detail::randomGenerator().randomDirectionQuat();
	}

	Vec4i randomRange4i(sint32 a, sint32 b)
	{
		return detail::randomGenerator().randomRange4i(a, b);
	}
}
