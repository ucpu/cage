#ifndef guard_renderPipeline_h_4hg1s8596drfh4
#define guard_renderPipeline_h_4hg1s8596drfh4

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API RenderPipeline : private Immovable
	{
	public:
		Vec2i windowResolution;
		uint64 time = 0;
		uint32 frameIndex = 0;
		Real interpolationFactor;

		Holder<RenderQueue> render();
	};

	struct CAGE_ENGINE_API RenderPipelineCreateConfig
	{
		AssetManager *assets = nullptr;
		ProvisionalGraphics *provisionalGraphics = nullptr;
		EntityManager *scene = nullptr;
	};

	CAGE_ENGINE_API Holder<RenderPipeline> newRenderPipeline(const RenderPipelineCreateConfig &config);
}

#endif // guard_renderPipeline_h_4hg1s8596drfh4
