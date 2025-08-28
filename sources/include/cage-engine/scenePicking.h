#ifndef guard_scenePicking_h_dzjlhgfds
#define guard_scenePicking_h_dzjlhgfds

#include <cage-core/entities.h>
#include <cage-core/geometry.h>
#include <cage-engine/scene.h>

namespace cage
{
	class Mesh;

	struct CAGE_ENGINE_API PickableComponent
	{};

	/*
	enum class ScenePickingPrecisionEnum
	{
		None = 0,
		BoundingBoxesOnly,
		BoundingMeshesOnly,
		Models,
		ModelsWithAlphaCut,
		ModelsWithAnimations,
		ModelsWithAnimationsAndAlphaCut,
	};
	*/

	struct CAGE_ENGINE_API ScenePickingConfig
	{
		Line picker;
		const AssetsManager *assets = nullptr;
		const EntityManager *entities = nullptr;
		//ScenePickingPrecisionEnum precision = ScenePickingPrecisionEnum::Models;

		// used for lod selection
		const Entity *camera = nullptr;
		uint32 screenHeight = 0;
	};

	struct CAGE_ENGINE_API ScenePickingResult
	{
		Vec3 point;
		Entity *entity = nullptr;
	};

	CAGE_ENGINE_API Holder<PointerRange<ScenePickingResult>> scenePicking(const ScenePickingConfig &config);
}

#endif // guard_scenePicking_h_dzjlhgfds
