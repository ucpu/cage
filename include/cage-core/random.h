#ifndef guard_random_h_623364ED17804404AAC89652473FEBAC
#define guard_random_h_623364ED17804404AAC89652473FEBAC

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API RandomGenerator
	{
		// https://en.wikipedia.org/wiki/Xorshift
		// xorshift128+
		uint64 s[2] { 0, 0 };

		RandomGenerator();
		explicit RandomGenerator(uint64 s[2]);
		explicit RandomGenerator(uint64 s1, uint64 s2);
		uint64 next();

		real randomChance();
		rads randomAngle();
		vec2 randomDirection2();
		vec3 randomDirection3();
		quat randomDirectionQuat();
#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max);
		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
		GCHL_GENERATE(real);
		GCHL_GENERATE(rads);
		GCHL_GENERATE(degs);
		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
#undef GCHL_GENERATE
	};

	namespace detail
	{
		CAGE_CORE_API RandomGenerator &getApplicationRandomGenerator();
	}
}

#endif // guard_random_h_623364ED17804404AAC89652473FEBAC
