#ifndef guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
#define guard_screenSpaceEffectsProperties_h_0156sdrf4treth1

#include <cage-engine/core.h>

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
		Real darkerSpeed = 1;
		Real lighterSpeed = 2;
		Real lowLogLum = -12;
		Real highLogLum = 2;
		Real nightOffset = 3;
		Real nightDesaturate = 0.2;
		Real nightContrast = 0.01;
		Real key = 0.1; // target gray
		Real strength = 0.8; // whole effect factor
	};

	struct CAGE_ENGINE_API ScreenSpaceBloom
	{
		uint32 blurPasses = 5;
		Real threshold = 1;
	};
}

#endif // guard_screenSpaceEffectsProperties_h_0156sdrf4treth1
