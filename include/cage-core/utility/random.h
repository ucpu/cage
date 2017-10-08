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

		real random();
		sint32 random(sint32 min, sint32 max);
		real random(real min, real max);
		rads random(rads min, rads max);
		rads randomAngle();
		vec2 randomDirection2();
		vec3 randomDirection3();
		quat randomDirectionQuat();
	};
}

#endif // guard_random_h_623364ED17804404AAC89652473FEBAC
