#include <cage-core/entities.h>
#include <cage-core/entitiesSerialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
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
			if (id == 0 || id == m)
				CAGE_THROW_ERROR(Exception, "cannot import anonymous entity");
			Entity *e = manager->getOrCreate(id);
			char *u = (char *)e->unsafeValue(component);
			des.read({ u, u + typeSize });
		}
	}
}
