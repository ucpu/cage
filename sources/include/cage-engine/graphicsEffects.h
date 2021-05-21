#ifndef guard_graphicsEffects_h_xcfvh241448960sdrt
#define guard_graphicsEffects_h_xcfvh241448960sdrt

#include "graphicsEffectsProperties.h"
#include "provisionalRenderHandles.h"

namespace cage
{
	struct CAGE_ENGINE_API GfCommonConfig
	{
		RenderQueue *queue = nullptr;
		AssetManager *assets = nullptr;
		ProvisionalRenderData *provisionals = nullptr;
		ivec2 resolution;
	};

	struct CAGE_ENGINE_API GfSsaoConfig : public GfCommonConfig, public GfSsao
	{
		mat4 viewProj;
		TextureHandle inDepth;
		TextureHandle inNormal;
		TextureHandle outAo;
		uint32 frameIndex = 0;
	};

	struct CAGE_ENGINE_API GfDepthOfFieldConfig : public GfCommonConfig, public GfDepthOfField
	{
		mat4 proj;
		TextureHandle inDepth;
		TextureHandle inColor;
		TextureHandle outColor;
	};

	struct CAGE_ENGINE_API GfMotionBlurConfig : public GfCommonConfig, public GfMotionBlur
	{
		TextureHandle inVelocity;
		TextureHandle inColor;
		TextureHandle outColor;
	};

	struct CAGE_ENGINE_API GfEyeAdaptationConfig : public GfCommonConfig, public GfEyeAdaptation
	{
		detail::StringBase<16> cameraId; // for synchronizing data across frames
		TextureHandle inColor; // used in both passes
		TextureHandle outColor; // used in the apply pass only
	};

	struct CAGE_ENGINE_API GfBloomConfig : public GfCommonConfig, public GfBloom
	{
		TextureHandle inColor;
		TextureHandle outColor;
	};

	struct CAGE_ENGINE_API GfTonemapConfig : public GfCommonConfig, public GfTonemap
	{
		TextureHandle inColor;
		TextureHandle outColor;
		real gamma = 2.2;
		bool tonemapEnabled = true;
	};

	struct CAGE_ENGINE_API GfFxaaConfig : public GfCommonConfig
	{
		TextureHandle inColor;
		TextureHandle outColor;
	};

	struct CAGE_ENGINE_API GfGaussianBlurConfig : public GfCommonConfig
	{
		TextureHandle texture;
		uint32 internalFormat = 0;
		uint32 mipmapLevel = 0;
	};

	CAGE_ENGINE_API void gfSsao(const GfSsaoConfig &config);
	CAGE_ENGINE_API void gfDepthOfField(const GfDepthOfFieldConfig &config);
	CAGE_ENGINE_API void gfMotionBlur(const GfMotionBlurConfig &config);
	CAGE_ENGINE_API void gfEyeAdaptationPrepare(const GfEyeAdaptationConfig &config);

	CAGE_ENGINE_API void gfBloom(const GfBloomConfig &config);
	CAGE_ENGINE_API void gfEyeAdaptationApply(const GfEyeAdaptationConfig &config);
	CAGE_ENGINE_API void gfTonemap(const GfTonemapConfig &config);
	CAGE_ENGINE_API void gfFxaa(const GfFxaaConfig &config);

	CAGE_ENGINE_API void gfGaussianBlur(const GfGaussianBlurConfig &config);
}

#endif // guard_graphicsEffects_h_xcfvh241448960sdrt
