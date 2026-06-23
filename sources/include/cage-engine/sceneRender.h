#ifndef guard_sceneRender_h_4hg1s8596drfh4
#define guard_sceneRender_h_4hg1s8596drfh4

#include <cage-engine/lodSelection.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneScreenSpaceEffects.h>

namespace cage
{
	class GraphicsDevice;
	class GraphicsEncoder;
	class AssetsOnDemand;
	class EntityManager;

	struct CAGE_ENGINE_API SceneRenderShared
	{
		uint64 currentTime = 0;
		uint64 elapsedTime = 1'000'000 / 60; // microseconds since last frame
		GraphicsDevice *device = nullptr;
		AssetsManager *assets = nullptr;
		AssetsOnDemand *onDemand = nullptr;
		EntityManager *scene = nullptr;
		uint32 frameIndex = 0;
		Real interpolationFactor = 1;
	};

	struct CAGE_ENGINE_API SceneRenderCamera
	{
		Mat4 projection;
		ScreenSpaceEffectsComponent effects;
		CameraCommonProperties camera;
		Transform transform;
		LodSelection lodSelection;
		Vec2i resolution;
		Texture *target = nullptr;
		uint32 cameraSceneMask = 1;
	};

	struct CAGE_ENGINE_API SceneRenderConfig
	{
		SceneRenderShared shared;
		PointerRange<const SceneRenderCamera> cameras;
	};

	CAGE_ENGINE_API Holder<PointerRange<Holder<GraphicsEncoder>>> sceneRender(const SceneRenderConfig &config);
}

#endif // guard_sceneRender_h_4hg1s8596drfh4
