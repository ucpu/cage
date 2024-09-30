#ifndef guard_sceneScreenSpaceEffects_h_xcvhj4n85rb
#define guard_sceneScreenSpaceEffects_h_xcvhj4n85rb

#include <cage-engine/screenSpaceEffectsProperties.h>

namespace cage
{
	enum class ScreenSpaceEffectsFlags : uint32
	{
		None = 0,
		AmbientOcclusion = 1 << 0,
		DepthOfField = 1 << 1,
		Bloom = 1 << 3,
		EyeAdaptation = 1 << 4,
		ToneMapping = 1 << 5,
		GammaCorrection = 1 << 6,
		AntiAliasing = 1 << 7,
		Default = AmbientOcclusion | Bloom | ToneMapping | GammaCorrection | AntiAliasing,
	};

	struct CAGE_ENGINE_API ScreenSpaceEffectsComponent
	{
		ScreenSpaceAmbientOcclusion ssao;
		ScreenSpaceBloom bloom;
		ScreenSpaceEyeAdaptation eyeAdaptation;
		ScreenSpaceDepthOfField depthOfField;
		Real gamma = 2.2;
		ScreenSpaceEffectsFlags effects = ScreenSpaceEffectsFlags::Default;
	};
}

#endif // guard_sceneScreenSpaceEffects_h_xcvhj4n85rb
