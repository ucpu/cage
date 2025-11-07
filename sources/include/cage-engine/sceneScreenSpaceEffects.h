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
		Bloom = 1 << 2,
		ToneMapping = 1 << 3,
		GammaCorrection = 1 << 4,
		AntiAliasing = 1 << 5,
		Sharpening = 1 << 6,
		Default = AmbientOcclusion | Bloom | ToneMapping | GammaCorrection | AntiAliasing,
	};

	struct CAGE_ENGINE_API ScreenSpaceEffectsComponent
	{
		ScreenSpaceAmbientOcclusion ssao;
		ScreenSpaceBloom bloom;
		ScreenSpaceDepthOfField depthOfField;
		ScreenSpaceSharpening sharpening;
		Real gamma = 2.2;
		ScreenSpaceEffectsFlags effects = ScreenSpaceEffectsFlags::Default;
	};
}

#endif // guard_sceneScreenSpaceEffects_h_xcvhj4n85rb
