#ifndef guard_screenSpaceEffects_h_xcfvh241448960sdrt
#define guard_screenSpaceEffects_h_xcfvh241448960sdrt

#include <cage-engine/screenSpaceEffectsProperties.h>

namespace cage
{
	class GraphicsEncoder;
	class GraphicsAggregateBuffer;
	class Texture;

	struct CAGE_ENGINE_API ScreenSpaceCommonConfig
	{
		GraphicsEncoder *encoder = nullptr;
		GraphicsAggregateBuffer *aggregate = nullptr;
		AssetsManager *assets = nullptr;
		Vec2i resolution;
	};

	struct CAGE_ENGINE_API ScreenSpaceAmbientOcclusionConfig : public ScreenSpaceCommonConfig, public ScreenSpaceAmbientOcclusion
	{
		Mat4 proj;
		Texture *inDepth = nullptr;
		Texture *outAo = nullptr;
		uint32 frameIndex = 0;
	};

	struct CAGE_ENGINE_API ScreenSpaceDepthOfFieldConfig : public ScreenSpaceCommonConfig, public ScreenSpaceDepthOfField
	{
		Mat4 proj;
		Texture *inDepth = nullptr;
		Texture *inColor = nullptr;
		Texture *outColor = nullptr;
	};

	struct CAGE_ENGINE_API ScreenSpaceBloomConfig : public ScreenSpaceCommonConfig, public ScreenSpaceBloom
	{
		Texture *inColor = nullptr;
		Texture *outColor = nullptr;
	};

	struct CAGE_ENGINE_API ScreenSpaceTonemapConfig : public ScreenSpaceCommonConfig
	{
		Texture *inColor = nullptr;
		Texture *outColor = nullptr;
		Real gamma = 2.2;
		bool tonemapEnabled = true;
	};

	struct CAGE_ENGINE_API ScreenSpaceFastApproximateAntiAliasingConfig : public ScreenSpaceCommonConfig
	{
		Texture *inColor = nullptr;
		Texture *outColor = nullptr;
	};

	struct CAGE_ENGINE_API ScreenSpaceSharpeningConfig : public ScreenSpaceCommonConfig, public ScreenSpaceSharpening
	{
		Texture *inColor = nullptr;
		Texture *outColor = nullptr;
	};

	CAGE_ENGINE_API void screenSpaceAmbientOcclusion(const ScreenSpaceAmbientOcclusionConfig &config);
	CAGE_ENGINE_API void screenSpaceDepthOfField(const ScreenSpaceDepthOfFieldConfig &config);
	CAGE_ENGINE_API void screenSpaceBloom(const ScreenSpaceBloomConfig &config);
	CAGE_ENGINE_API void screenSpaceTonemap(const ScreenSpaceTonemapConfig &config);
	CAGE_ENGINE_API void screenSpaceFastApproximateAntiAliasing(const ScreenSpaceFastApproximateAntiAliasingConfig &config);
	CAGE_ENGINE_API void screenSpaceSharpening(const ScreenSpaceSharpeningConfig &config);
}

#endif // guard_screenSpaceEffects_h_xcfvh241448960sdrt
