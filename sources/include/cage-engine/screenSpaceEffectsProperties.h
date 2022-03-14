#ifndef guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
#define guard_screenSpaceEffectsProperties_h_0156sdrf4treth1

#include "core.h"

namespace cage
{
	struct CAGE_ENGINE_API ScreenSpaceAmbientOcclusion
	{
		Real worldRadius = 0.5;
		Real bias = 0.03;
		Real power = 1.3;
		Real strength = 3;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount = 16;
		uint32 blurPasses = 3;
	};

	struct CAGE_ENGINE_API ScreenSpaceDepthOfField
	{
		// objects within (focusDistance - focusRadius) and (focusDistance + focusRadius) are in focus
		// objects further than focusDistance + focusRadius + blendRadius are out of focus
		// objects closer than focusDistance - focusRadius - blendRadius are out of focus
		Real focusDistance = 5;
		Real focusRadius = 0;
		Real blendRadius = 5;
		uint32 blurPasses = 3;
	};

	struct CAGE_ENGINE_API ScreenSpaceEyeAdaptation
	{
		Real key = 0.15;
		Real strength = 0.5;
		Real darkerSpeed = 0.2;
		Real lighterSpeed = 1;
	};

	struct CAGE_ENGINE_API ScreenSpaceBloom
	{
		uint32 blurPasses = 5;
		Real threshold = 1;
	};

	struct CAGE_ENGINE_API ScreenSpaceTonemap
	{
		Real shoulderStrength = 0.22;
		Real linearStrength = 0.3;
		Real linearAngle = 0.1;
		Real toeStrength = 0.2;
		Real toeNumerator = 0.01;
		Real toeDenominator = 0.3;
		Real white = 11.2;
	};
}

#endif // guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
