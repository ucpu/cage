#include <set>
#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utility/memoryBuffer.h>

namespace cage
{
	namespace
	{
		void write(pointer &ptr, uintPtr &bufferSize, void *what, uintPtr size)
		{
			if (bufferSize < size)
				CAGE_THROW_ERROR(exception, "not enough space");
			detail::memcpy(ptr.asVoid, what, size);
			ptr += size;
			bufferSize -= size;
		}

		template<class T> void write(pointer &ptr, uintPtr &bufferSize, T what)
		{
			write(ptr, bufferSize, &what, sizeof(T));
		}

		void read(pointer &ptr, uintPtr &bufferSize, void *what, uintPtr size)
		{
			if (bufferSize < size)
				CAGE_THROW_ERROR(exception, "not enough space");
			detail::memcpy(what, ptr.asVoid, size);
			ptr += size;
			bufferSize -= size;
		}

		template<class T> T read(pointer &ptr, uintPtr &bufferSize)
		{
			T t;
			read(ptr, bufferSize, &t, sizeof(T));
			return t;
		}
	}

	uintPtr entitiesSerialize(void *buffer, uintPtr bufferSize, groupClass *entities, componentClass *component)
	{
		uint32 ec = entities->entitiesCount();
		if (!ec)
			return 0;
		uintPtr typeSize = component->getTypeSize();
		pointer ptr = buffer;
		write(ptr, bufferSize, component->getComponentIndex());
		write(ptr, bufferSize, (uint64)typeSize);
		uint32 &cnt = *ptr.asUint32;
		write(ptr, bufferSize, uint32(0));
		entityClass *const *ents = entities->entitiesArray();
		while (ec--)
		{
			entityClass *e = *ents++;
			uint32 name = e->getName();
			if (name == 0)
				continue;
			if (!e->hasComponent(component))
				continue;
			cnt++;
			write(ptr, bufferSize, name);
			write(ptr, bufferSize, e->unsafeValue(component), typeSize);
		}
		return ptr - buffer;
	}

	memoryBuffer entitiesSerialize(groupClass *entities, componentClass *component)
	{
		memoryBuffer buff(100 + entities->entitiesCount()  * component->getTypeSize());
		auto res = entitiesSerialize(buff.data(), buff.size(), entities, component);
		buff.resize(res);
		return buff;
	}

	void entitiesDeserialize(const void *buffer, uintPtr bufferSize, entityManagerClass *manager)
	{
		pointer ptr = const_cast<void*>(buffer);
		uint32 componentIndex = read<uint32>(ptr, bufferSize);
		if (componentIndex >= manager->getComponentsCount())
			CAGE_THROW_ERROR(exception, "incompatible component (different index)");
		componentClass *component = manager->getComponentByIndex(componentIndex);
		uint64 typeSize = read<uint64>(ptr, bufferSize);
		if (component->getTypeSize() != typeSize)
			CAGE_THROW_ERROR(exception, "incompatible component (different size)");
		uint32 cnt = read<uint32>(ptr, bufferSize);
		while (cnt--)
		{
			uint32 name = read<uint32>(ptr, bufferSize);
			if (name == 0)
				CAGE_THROW_ERROR(exception, "invalid entity");
			if (!manager->hasEntity(name))
				manager->newEntity(name);
			entityClass *e = manager->getEntity(name);
			read(ptr, bufferSize, e->unsafeValue(component), typeSize);
		}
	}

	void entitiesDeserialize(const memoryBuffer &buffer, entityManagerClass *manager)
	{
		entitiesDeserialize(buffer.data(), buffer.size(), manager);
	}
}
