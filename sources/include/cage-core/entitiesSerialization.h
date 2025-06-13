#ifndef guard_entitiesSerialization_sdrgdfzj5hsdet
#define guard_entitiesSerialization_sdrgdfzj5hsdet

#include <cage-core/core.h>

namespace cage
{
	class EntityManager;
	class EntityComponent;
	class Entity;

	CAGE_CORE_API Holder<PointerRange<char>> entitiesExportBuffer(PointerRange<Entity *const> entities, EntityComponent *component);
	CAGE_CORE_API void entitiesImportBuffer(PointerRange<const char> buffer, EntityManager *manager);
}

#endif // guard_entitiesSerialization_sdrgdfzj5hsdet
