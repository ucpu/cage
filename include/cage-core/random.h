#ifndef guard_random_h_623364ED17804404AAC89652473FEBAC
#define guard_random_h_623364ED17804404AAC89652473FEBAC

namespace cage
{
	struct CAGE_API RandomGenerator
	{
		// https://en.wikipedia.org/wiki/Xorshift
		// xorshift128+
		uint64 s[2];

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
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, degs, float, double));
#undef GCHL_GENERATE
	};

	namespace detail
	{
		CAGE_API RandomGenerator &getApplicationRandomGenerator();
	}
}

#endif // guard_random_h_623364ED17804404AAC89652473FEBAC
