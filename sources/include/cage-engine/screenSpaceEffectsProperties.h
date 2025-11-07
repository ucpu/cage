#ifndef guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
#define guard_screenSpaceEffectsProperties_h_0156sdrf4treth1

#include <cage-engine/core.h>

namespace cage
{
	struct CAGE_ENGINE_API ScreenSpaceAmbientOcclusion
	{
		Real raysLength = 0.1; // world-space
		Real bias = 0.03;
		Real power = 1.2;
		Real strength = 3;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount = 32;
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

	struct CAGE_ENGINE_API ScreenSpaceBloom
	{
		uint32 blurPasses = 5;
		Real threshold = 1;
	};

	struct CAGE_ENGINE_API ScreenSpaceSharpening
	{
		Real strength = 1;
	};
}

#endif // guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
