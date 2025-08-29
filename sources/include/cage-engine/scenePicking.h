#ifndef guard_scenePicking_h_dzjlhgfds
#define guard_scenePicking_h_dzjlhgfds

#include <cage-core/entities.h>
#include <cage-core/geometry.h>
#include <cage-engine/lodSelection.h>

namespace cage
{
	struct CAGE_ENGINE_API PickableComponent
	{};

	struct CAGE_ENGINE_API ScenePickingConfig
	{
		Line picker;
		LodSelection lodSelection;
		const AssetsManager *assets = nullptr;
		const EntityManager *entities = nullptr;
	};

	struct CAGE_ENGINE_API ScenePickingResult
	{
		Vec3 point;
		Entity *entity = nullptr;
	};

	CAGE_ENGINE_API Holder<PointerRange<ScenePickingResult>> scenePicking(const ScenePickingConfig &config);
}

#endif // guard_scenePicking_h_dzjlhgfds
