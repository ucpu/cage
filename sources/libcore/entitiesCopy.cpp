#include <cage-core/entities.h>
#include <cage-core/entitiesCopy.h>

#include <map>

namespace cage
{
	namespace
	{
		std::map<EntityComponent *, EntityComponent *> mapping(const EntitiesCopyConfig &config)
		{
			std::map<EntityComponent *, EntityComponent *> res;
			for (EntityComponent *sc : config.source->components())
			{
				uint32 idx = 0;
				for (EntityComponent *dc : config.source->componentsByType(sc->typeIndex()))
				{
					if (sc == dc)
						break;
					else
						idx++;
				}
				auto cbts = config.destination->componentsByType(sc->typeIndex());
				if (idx >= cbts.size())
				{
					const uint32 k = idx - cbts.size() + 1;
					for (uint32 i = 0; i < k; i++)
						config.destination->defineComponent(sc);
					cbts = config.destination->componentsByType(sc->typeIndex());
				}
				res[sc] = cbts[idx];
			}
			return res;
		}
	}

	void entitiesCopy(const EntitiesCopyConfig &config)
	{
		const auto mp = mapping(config);
		config.destination->destroy();
		for (Entity *se : config.source->entities())
		{
			Entity *de = se->name() ? config.destination->create(se->name()) : config.destination->createAnonymous();
			for (const auto &it : mp)
			{
				if (!se->has(it.first))
					continue;
				detail::memcpy(de->unsafeValue(it.second), se->unsafeValue(it.first), detail::typeSize(it.first->typeIndex()));
			}
		}
	}
}

