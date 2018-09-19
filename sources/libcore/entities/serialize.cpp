#include <set>
#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	memoryBuffer entitiesSerialize(groupClass *entities, componentClass *component)
	{
		uintPtr typeSize = component->getTypeSize();
		memoryBuffer buffer;
		serializer ser(buffer);
		ser << component->getComponentIndex();
		ser << (uint64)typeSize;
		ser << (uint32)0; // cnt placeholder
		uint32 cnt = 0;
		for (entityClass *e : entities->entities())
		{
			uint32 name = e->getName();
			if (name == 0)
				continue;
			if (!e->hasComponent(component))
				continue;
			cnt++;
			ser << name;
			ser.write(e->unsafeValue(component), typeSize);
		}
		*(uint32*)(buffer.data() + sizeof(component->getComponentIndex()) + sizeof(uint64)) = cnt;
		return buffer;
	}

	void entitiesDeserialize(const void *buffer, uintPtr size, entityManagerClass *manager)
	{
		deserializer des(buffer, size);
		uint32 componentIndex;
		des >> componentIndex;
		if (componentIndex >= manager->getComponentsCount())
			CAGE_THROW_ERROR(exception, "incompatible component (different index)");
		componentClass *component = manager->getComponentByIndex(componentIndex);
		uint64 typeSize;
		des >> typeSize;
		if (component->getTypeSize() != typeSize)
			CAGE_THROW_ERROR(exception, "incompatible component (different size)");
		uint32 cnt;
		des >> cnt;
		while (cnt--)
		{
			uint32 name;
			des >> name;
			if (name == 0)
				CAGE_THROW_ERROR(exception, "invalid entity");
			entityClass *e = manager->getOrNewEntity(name);
			des.read(e->unsafeValue(component), numeric_cast<uintPtr>(typeSize));
		}
	}

	void entitiesDeserialize(const memoryBuffer &buffer, entityManagerClass *manager)
	{
		entitiesDeserialize(buffer.data(), buffer.size(), manager);
	}
}
