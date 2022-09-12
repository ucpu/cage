#ifndef guard_renderPipeline_h_4hg1s8596drfh4
#define guard_renderPipeline_h_4hg1s8596drfh4

#include "camera.h"
#include "provisionalHandles.h"

#include <variant>

namespace cage
{
	class EntityManager;
	class ShaderProgram;
	class RenderQueue;
	class ProvisionalGraphics;

	struct CAGE_ENGINE_API RenderPipelineCamera
	{
		String name;
		TextureHandle target;
		CameraProperties camera;
		Transform transform;
		Vec2i resolution;
	};

	struct CAGE_ENGINE_API RenderPipelineDebugVisualization
	{
		TextureHandle texture;
		Holder<ShaderProgram> shader;
	};

	struct CAGE_ENGINE_API RenderPipelineResult
	{
		Holder<PointerRange<RenderPipelineDebugVisualization>> debugVisualizations;
		Holder<RenderQueue> renderQueue;
	};

	class CAGE_ENGINE_API RenderPipeline : private Immovable
	{
	public:
		uint64 time = 0;
		uint32 frameIndex = 0;
		Real interpolationFactor = 1;

		bool reinitialize();

		// thread-safe: you may render multiple cameras simultaneously in threads
		RenderPipelineResult render(const RenderPipelineCamera &camera);
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
