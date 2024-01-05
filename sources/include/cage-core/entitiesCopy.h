#ifndef guard_entitiesCopy_4jh1gv89sert74hz
#define guard_entitiesCopy_4jh1gv89sert74hz

#include <cage-core/core.h>

namespace cage
{
	class EntityManager;
	class EntityComponent;
	class Entity;

	struct CAGE_CORE_API EntitiesCopyConfig
	{
		const EntityManager *source = nullptr;
		EntityManager *destination = nullptr;
	};

	CAGE_CORE_API void entitiesCopy(const EntitiesCopyConfig &config);

	CAGE_CORE_API Holder<PointerRange<char>> entitiesExportBuffer(PointerRange<Entity *const> entities, EntityComponent *component);
	CAGE_CORE_API void entitiesImportBuffer(PointerRange<const char> buffer, EntityManager *manager);
}

#endif // guard_entitiesCopy_4jh1gv89sert74hz
