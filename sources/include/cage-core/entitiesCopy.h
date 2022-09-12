#ifndef guard_entitiesCopy_4jh1gv89sert74hz
#define guard_entitiesCopy_4jh1gv89sert74hz

#include "core.h"

namespace cage
{
	class EntityManager;

	struct CAGE_CORE_API EntitiesCopyConfig
	{
		const EntityManager *source = nullptr;
		EntityManager *destination = nullptr;
	};

	CAGE_CORE_API void entitiesCopy(const EntitiesCopyConfig &config);
}

#endif // guard_entitiesCopy_4jh1gv89sert74hz
