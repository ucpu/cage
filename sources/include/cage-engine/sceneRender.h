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

	struct CAGE_ENGINE_API ScenePrepareConfig
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

	struct CAGE_ENGINE_API SceneCameraConfig
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

	struct CAGE_ENGINE_API PreparedScene
	{
		const ScenePrepareConfig config;
	};

	CAGE_ENGINE_API Holder<PreparedScene> scenePrepare(const ScenePrepareConfig &config);
	CAGE_ENGINE_API Holder<PointerRange<Holder<GraphicsEncoder>>> sceneRender(const PreparedScene *scene, const SceneCameraConfig &config);
}

#endif // guard_sceneRender_h_4hg1s8596drfh4
