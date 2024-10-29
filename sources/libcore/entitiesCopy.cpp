#include <map>

#include <cage-core/entities.h>
#include <cage-core/entitiesCopy.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

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
			Entity *de = se->id() ? config.destination->create(se->id()) : config.destination->createAnonymous();
			for (const auto &it : mp)
			{
				if (!se->has(it.first))
					continue;
				detail::memcpy(de->unsafeValue(it.second), se->unsafeValue(it.first), detail::typeSizeByIndex(it.first->typeIndex()));
			}
		}
	}

	Holder<PointerRange<char>> entitiesExportBuffer(PointerRange<Entity *const> entities, EntityComponent *component)
	{
		const uintPtr typeSize = detail::typeSizeByIndex(component->typeIndex());
		MemoryBuffer buffer;
		Serializer ser(buffer);
		ser << component->definitionIndex();
		ser << (uint64)typeSize;
		Serializer cntPlaceholder = ser.reserve(sizeof(uint32));
		uint32 cnt = 0;
		for (Entity *e : entities)
		{
			CAGE_ASSERT(e->manager() == component->manager());
			uint32 id = e->id();
			if (id == 0)
				continue;
			if (!e->has(component))
				continue;
			cnt++;
			ser << id;
			const char *u = (const char *)e->unsafeValue(component);
			ser.write({ u, u + typeSize });
		}
		if (cnt == 0)
			return {};
		cntPlaceholder << cnt;
		return std::move(buffer);
	}

	void entitiesImportBuffer(PointerRange<const char> buffer, EntityManager *manager)
	{
		if (buffer.empty())
			return;
		Deserializer des(buffer);
		uint32 componentIndex;
		des >> componentIndex;
		if (componentIndex >= manager->componentsCount())
			CAGE_THROW_ERROR(Exception, "incompatible component (different index)");
		EntityComponent *component = manager->componentByDefinition(componentIndex);
		uint64 typeSize;
		des >> typeSize;
		if (detail::typeSizeByIndex(component->typeIndex()) != typeSize)
			CAGE_THROW_ERROR(Exception, "incompatible component (different size)");
		uint32 cnt;
		des >> cnt;
		while (cnt--)
		{
			uint32 id;
			des >> id;
			if (id == 0)
				CAGE_THROW_ERROR(Exception, "anonymous entity");
			Entity *e = manager->getOrCreate(id);
			char *u = (char *)e->unsafeValue(component);
			des.read({ u, u + typeSize });
		}
	}
}
