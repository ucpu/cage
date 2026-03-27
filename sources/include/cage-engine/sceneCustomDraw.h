#ifndef guard_sceneCustomDraw_srui74ko8er
#define guard_sceneCustomDraw_srui74ko8er

#include <cage-engine/core.h>

namespace cage
{
	class Entity;
	class GraphicsEncoder;
	class GraphicsAggregateBuffer;
	struct ScenePrepareConfig;
	struct SceneCameraConfig;

	struct CAGE_ENGINE_API CustomDrawConfig
	{
		const ScenePrepareConfig *sceneConfig = nullptr;
		const SceneCameraConfig *cameraConfig = nullptr;
		GraphicsEncoder *encoder = nullptr;
		GraphicsAggregateBuffer *aggregate = nullptr;
		Entity *entity = nullptr;
	};

	struct CAGE_ENGINE_API CustomDrawComponent
	{
		Delegate<void(const CustomDrawConfig &)> callback;
		sint32 renderLayer = 0;
	};
}

#endif // guard_sceneCustomDraw_srui74ko8er
