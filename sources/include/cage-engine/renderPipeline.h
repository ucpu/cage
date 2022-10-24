#ifndef guard_renderPipeline_h_4hg1s8596drfh4
#define guard_renderPipeline_h_4hg1s8596drfh4

#include "scene.h"
#include "sceneScreenSpaceEffects.h"
#include "provisionalHandles.h"

namespace cage
{
	class EntityManager;
	class ShaderProgram;
	class RenderQueue;
	class ProvisionalGraphics;

	struct LodSelection
	{
		Vec3 center; // center of camera
		Real screenSize = 0; // vertical size of screen in pixels, one meter in front of the camera
		bool orthographic = false;
	};

	struct CAGE_ENGINE_API RenderPipelineCamera
	{
		CameraCommonProperties camera;
		ScreenSpaceEffectsComponent effects;
		LodSelection lodSelection;
		String name;
		Mat4 projection;
		Transform transform;
		TextureHandle target;
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
		uint64 currentTime = 0;
		uint64 elapsedTime = 1000000 / 60; // microseconds since last frame
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
