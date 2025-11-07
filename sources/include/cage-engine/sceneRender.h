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

	struct CAGE_ENGINE_API SceneRenderConfig
	{
		ScreenSpaceEffectsComponent effects;
		Mat4 projection;
		CameraCommonProperties camera;
		Transform transform;
		LodSelection lodSelection;
		uint64 currentTime = 0;
		uint64 elapsedTime = 1'000'000 / 60; // microseconds since last frame
		Vec2i resolution;
		GraphicsDevice *device = nullptr;
		AssetsManager *assets = nullptr;
		AssetsOnDemand *onDemand = nullptr;
		EntityManager *scene = nullptr;
		Texture *target = nullptr;
		uint32 frameIndex = 0;
		uint32 cameraSceneMask = 1;
		Real interpolationFactor = 1;
	};

	CAGE_ENGINE_API Holder<PointerRange<Holder<GraphicsEncoder>>> sceneRender(const SceneRenderConfig &config);
}

#endif // guard_sceneRender_h_4hg1s8596drfh4
