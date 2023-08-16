#ifndef guard_random_h_623364ED17804404AAC89652473FEBAC
#define guard_random_h_623364ED17804404AAC89652473FEBAC

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API RandomGenerator
	{
		// https://en.wikipedia.org/wiki/Xorshift
		// xorshift128+

		uint64 s[2] = {};

		RandomGenerator(); // initialize with random seed
		explicit RandomGenerator(uint64 s1, uint64 s2);
		uint64 next();

		Real randomChance(); // 0 to 1; including 0, excluding 1
		double randomChanceDouble(); // 0 to 1; including 0, excluding 1
#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max);
		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
		GCHL_GENERATE(Real);
		GCHL_GENERATE(Rads);
		GCHL_GENERATE(Degs);
#undef GCHL_GENERATE
		Rads randomAngle();
		Vec2 randomChance2();
		Vec2 randomRange2(Real a, Real b);
		Vec2 randomDirection2();
		Vec2i randomRange2i(sint32 a, sint32 b);
		Vec3 randomChance3();
		Vec3 randomRange3(Real a, Real b);
		Vec3 randomDirection3();
		Vec3i randomRange3i(sint32 a, sint32 b);
		Vec4 randomChance4();
		Vec4 randomRange4(Real a, Real b);
		Quat randomDirectionQuat();
		Vec4i randomRange4i(sint32 a, sint32 b);
	};

	namespace detail
	{
		// thread-local random generator used by all the global random functions
		CAGE_CORE_API RandomGenerator &randomGenerator();
	}
}

#endif // guard_random_h_623364ED17804404AAC89652473FEBAC
