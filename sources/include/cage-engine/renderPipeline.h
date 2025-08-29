#ifndef guard_renderPipeline_h_4hg1s8596drfh4
#define guard_renderPipeline_h_4hg1s8596drfh4

#include <cage-engine/lodSelection.h>
#include <cage-engine/provisionalHandles.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneScreenSpaceEffects.h>

namespace cage
{
	class EntityManager;
	class RenderQueue;
	class ProvisionalGraphics;
	class AssetsOnDemand;

	struct CAGE_ENGINE_API RenderPipelineConfig
	{
		String name;
		ScreenSpaceEffectsComponent effects;
		Mat4 projection;
		Transform transform;
		TextureHandle target;
		CameraCommonProperties camera;
		LodSelection lodSelection;
		uint64 currentTime = 0;
		uint64 elapsedTime = 1'000'000 / 60; // microseconds since last frame
		Vec2i resolution;
		AssetsManager *assets = nullptr;
		ProvisionalGraphics *provisionalGraphics = nullptr;
		EntityManager *scene = nullptr;
		AssetsOnDemand *onDemand = nullptr;
		uint32 frameIndex = 0;
		uint32 cameraSceneMask = 1;
		Real interpolationFactor = 1;
	};

	CAGE_ENGINE_API Holder<RenderQueue> renderPipeline(const RenderPipelineConfig &config);
}

#endif // guard_renderPipeline_h_4hg1s8596drfh4
