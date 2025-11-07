#ifndef guard_sceneCustomDraw_srui74ko8er
#define guard_sceneCustomDraw_srui74ko8er

#include <cage-engine/core.h>

namespace cage
{
	class Entity;
	class GraphicsEncoder;
	class GraphicsAggregateBuffer;
	struct SceneRenderConfig;

	struct CAGE_ENGINE_API CustomDrawConfig
	{
		const SceneRenderConfig *renderConfig = nullptr;
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
