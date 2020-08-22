#ifndef guard_random_h_623364ED17804404AAC89652473FEBAC
#define guard_random_h_623364ED17804404AAC89652473FEBAC

#include "core.h"

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

		real randomChance(); // 0 to 1; including 0, excluding 1
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
		GCHL_GENERATE(real);
		GCHL_GENERATE(rads);
		GCHL_GENERATE(degs);
#undef GCHL_GENERATE
		rads randomAngle();
		vec2 randomChance2();
		vec2 randomRange2(real a, real b);
		vec2 randomDirection2();
		ivec2 randomRange2i(sint32 a, sint32 b);
		vec3 randomChance3();
		vec3 randomRange3(real a, real b);
		vec3 randomDirection3();
		ivec3 randomRange3i(sint32 a, sint32 b);
		vec4 randomChance4();
		vec4 randomRange4(real a, real b);
		quat randomDirectionQuat();
		ivec4 randomRange4i(sint32 a, sint32 b);
	};

	namespace detail
	{
		CAGE_CORE_API RandomGenerator &getApplicationRandomGenerator();
	}
}

#endif // guard_random_h_623364ED17804404AAC89652473FEBAC
