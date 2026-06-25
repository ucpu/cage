#ifndef guard_scenePicking_h_dzjlhgfds
#define guard_scenePicking_h_dzjlhgfds

#include <cage-core/entities.h>
#include <cage-core/geometry.h>
#include <cage-engine/lodSelection.h>

namespace cage
{
	// mark an entity as valid target for picking
	// must have ModelComponent or SpriteComponent with Model with a collider
	struct CAGE_ENGINE_API PickableComponent
	{};

	struct CAGE_ENGINE_API ScenePickingResult
	{
		Vec3 point;
		Entity *entity = nullptr;
	};

	// contains cache of pickable entities
	// so create new one every frame
	class CAGE_ENGINE_API ScenePicking : private Immovable
	{
	public:
		Holder<PointerRange<ScenePickingResult>> pick(Line picker) const;
	};

	struct CAGE_ENGINE_API ScenePickingConfig
	{
		LodSelection lodSelection;
		const AssetsManager *assets = nullptr;
		const EntityManager *entities = nullptr;
		bool allowSpatialStructure = true; // disable if you do just one query
	};

	CAGE_ENGINE_API Holder<ScenePicking> newScenePicking(const ScenePickingConfig &config);
}

#endif // guard_scenePicking_h_dzjlhgfds
