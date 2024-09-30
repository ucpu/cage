#ifndef guard_screenSpaceEffects_h_xcfvh241448960sdrt
#define guard_screenSpaceEffects_h_xcfvh241448960sdrt

#include <cage-engine/provisionalHandles.h>
#include <cage-engine/screenSpaceEffectsProperties.h>

namespace cage
{
	class RenderQueue;
	class AssetManager;
	class ProvisionalGraphics;

	struct CAGE_ENGINE_API ScreenSpaceCommonConfig
	{
		RenderQueue *queue = nullptr;
		AssetManager *assets = nullptr;
		ProvisionalGraphics *provisionals = nullptr;
		Vec2i resolution;
	};

	struct CAGE_ENGINE_API ScreenSpaceAmbientOcclusionConfig : public ScreenSpaceCommonConfig, public ScreenSpaceAmbientOcclusion
	{
		Mat4 proj;
		TextureHandle inDepth;
		mutable TextureHandle outAo;
		uint32 frameIndex = 0;
	};

	struct CAGE_ENGINE_API ScreenSpaceDepthOfFieldConfig : public ScreenSpaceCommonConfig, public ScreenSpaceDepthOfField
	{
		Mat4 proj;
		TextureHandle inDepth;
		TextureHandle inColor;
		TextureHandle outColor;
	};

	struct CAGE_ENGINE_API ScreenSpaceEyeAdaptationConfig : public ScreenSpaceCommonConfig, public ScreenSpaceEyeAdaptation
	{
		String cameraId; // for synchronizing data across frames
		TextureHandle inColor; // used in both passes
		TextureHandle outColor; // used in the apply pass only
		Real elapsedTime = 1.0 / 60;
	};

	struct CAGE_ENGINE_API ScreenSpaceBloomConfig : public ScreenSpaceCommonConfig, public ScreenSpaceBloom
	{
		TextureHandle inColor;
		TextureHandle outColor;
	};

	struct CAGE_ENGINE_API ScreenSpaceTonemapConfig : public ScreenSpaceCommonConfig
	{
		TextureHandle inColor;
		TextureHandle outColor;
		Real gamma = 2.2;
		bool tonemapEnabled = true;
	};

	struct CAGE_ENGINE_API ScreenSpaceFastApproximateAntiAliasingConfig : public ScreenSpaceCommonConfig
	{
		TextureHandle inColor;
		TextureHandle outColor;
	};

	CAGE_ENGINE_API void screenSpaceAmbientOcclusion(const ScreenSpaceAmbientOcclusionConfig &config);
	CAGE_ENGINE_API void screenSpaceDepthOfField(const ScreenSpaceDepthOfFieldConfig &config);
	CAGE_ENGINE_API void screenSpaceEyeAdaptationPrepare(const ScreenSpaceEyeAdaptationConfig &config);
	CAGE_ENGINE_API void screenSpaceBloom(const ScreenSpaceBloomConfig &config);
	CAGE_ENGINE_API void screenSpaceEyeAdaptationApply(const ScreenSpaceEyeAdaptationConfig &config);
	CAGE_ENGINE_API void screenSpaceTonemap(const ScreenSpaceTonemapConfig &config);
	CAGE_ENGINE_API void screenSpaceFastApproximateAntiAliasing(const ScreenSpaceFastApproximateAntiAliasingConfig &config);
}

#endif // guard_screenSpaceEffects_h_xcfvh241448960sdrt
