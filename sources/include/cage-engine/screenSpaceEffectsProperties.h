#ifndef guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
#define guard_screenSpaceEffectsProperties_h_0156sdrf4treth1

#include "core.h"

namespace cage
{
	struct CAGE_ENGINE_API ScreenSpaceAmbientOcclusion
	{
		real worldRadius = 0.5;
		real bias = 0.03;
		real power = 1.3;
		real strength = 3;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount = 16;
		uint32 blurPasses = 3;
	};

	struct CAGE_ENGINE_API ScreenSpaceDepthOfField
	{
		// objects within (focusDistance - focusRadius) and (focusDistance + focusRadius) are in focus
		// objects further than focusDistance + focusRadius + blendRadius are out of focus
		// objects closer than focusDistance - focusRadius - blendRadius are out of focus
		real focusDistance = 5;
		real focusRadius = 0;
		real blendRadius = 5;
		uint32 blurPasses = 3;
	};

	struct CAGE_ENGINE_API ScreenSpaceMotionBlur
	{
		// todo
	};

	struct CAGE_ENGINE_API ScreenSpaceEyeAdaptation
	{
		real key = 0.15;
		real strength = 0.5;
		real darkerSpeed = 0.2;
		real lighterSpeed = 1;
	};

	struct CAGE_ENGINE_API ScreenSpaceBloom
	{
		uint32 blurPasses = 5;
		real threshold = 1;
	};

	struct CAGE_ENGINE_API ScreenSpaceTonemap
	{
		real shoulderStrength = 0.22;
		real linearStrength = 0.3;
		real linearAngle = 0.1;
		real toeStrength = 0.2;
		real toeNumerator = 0.01;
		real toeDenominator = 0.3;
		real white = 11.2;
	};
}

#endif // guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
