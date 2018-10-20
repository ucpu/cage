#ifndef guard_random_h_623364ED17804404AAC89652473FEBAC
#define guard_random_h_623364ED17804404AAC89652473FEBAC

namespace cage
{
	struct CAGE_API randomGenerator
	{
		// https://en.wikipedia.org/wiki/Xorshift
		// xorshift128+

		uint64 s[2];
		randomGenerator();
		randomGenerator(uint64 s[2]);
		randomGenerator(uint64 s1, uint64 s2);
		uint64 next();

		real randomChance();
		rads randomAngle();
		vec2 randomDirection2();
		vec3 randomDirection3();
		quat randomDirectionQuat();
#define GCHL_GENERATE(TYPE) TYPE randomRange(TYPE min, TYPE max);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, float, double));
#undef GCHL_GENERATE
	};

	CAGE_API randomGenerator &currentRandomGenerator();
}

#endif // guard_random_h_623364ED17804404AAC89652473FEBAC