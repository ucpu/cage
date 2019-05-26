#include <vector>
#include <set>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/hashTable.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	namespace
	{
		class entityManagerImpl;
		class componentImpl;
		class entityImpl;
		class groupImpl;

		class groupImpl : public groupClass
		{
		public:
			entityManagerImpl *const manager;
			std::vector<entityClass*> entities;
			const uint32 vectorIndex;

			groupImpl(entityManagerImpl *manager);
		};

		class entityManagerImpl : public entityManagerClass
		{
		public:
			std::vector<componentImpl*> components;
			std::vector<groupImpl*> groups;
			groupImpl allEntities;
			uint32 generateName;
			holder<hashTableClass<entityImpl>> namedEntities;

#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4355) // disable warning that using this in initializer list is dangerous
#endif

			entityManagerImpl(const entityManagerCreateConfig &config) : allEntities(this), generateName(0)
			{
				namedEntities = newHashTable<entityImpl>(1024, 1024 * 1024);
			}

#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (pop)
#endif

			~entityManagerImpl()
			{
				allEntities.destroy();
				for (auto it = groups.begin(), et = groups.end(); it != et; it++)
					detail::systemArena().destroy<groupImpl>(*it);
				for (auto it = components.begin(), et = components.end(); it != et; it++)
					detail::systemArena().destroy<componentImpl>(*it);
			}

			uint32 generateUniqueName()
			{
				static const uint32 a = (uint32)1 << 28;
				static const uint32 b = (uint32)1 << 30;
				if (generateName < a || generateName > b)
					generateName = a;
				while (has(generateName))
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}
		};

		groupImpl::groupImpl(entityManagerImpl *manager) : manager(manager), vectorIndex(numeric_cast<uint32>(manager->groups.size()))
		{
			entities.reserve(100);
		}

		class componentImpl : public componentClass
		{
		public:
			entityManagerImpl *const manager;
			holder<groupImpl> componentEntities;
			const uintPtr typeSize;
			const uintPtr typeAlignment;
			void *prototype;
			const uint32 vectorIndex;

			componentImpl(entityManagerImpl *manager, uintPtr typeSize, uintPtr typeAlignment, void *prototype, const componentCreateConfig &config) :
				manager(manager), typeSize(typeSize), typeAlignment(typeAlignment), vectorIndex(numeric_cast<uint32>(manager->components.size()))
			{
				this->prototype = detail::systemArena().allocate(typeSize, typeAlignment);
				detail::memcpy(this->prototype, prototype, typeSize);
				if (config.enumerableEntities)
					componentEntities = detail::systemArena().createHolder<groupImpl>(manager);
			}

			~componentImpl()
			{
				detail::systemArena().deallocate(prototype);
			}
		};

		class entityImpl : public entityClass
		{
		public:
			entityManagerImpl *const manager;
			const uint32 name;
			std::set<groupClass*> groups;
			std::vector<void*> components;

			entityImpl(entityManagerImpl *manager, uint32 name) :
				manager(manager), name(name)
			{
				if (name != 0)
					manager->namedEntities->add(name, this);
				manager->allEntities.add(this);
			}

			~entityImpl()
			{
				for (uint32 i = 0, e = numeric_cast<uint32>(components.size()); i != e; i++)
					if (components[i])
						remove(manager->components[i]);
				while (!groups.empty())
					remove(*groups.begin());
				if (name != 0)
					manager->namedEntities->remove(name);
			}
		};
	}

	componentClass *entityManagerClass::componentByIndex(uint32 index) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT_RUNTIME(index < impl->components.size());
		return impl->components[index];
	}

	groupClass *entityManagerClass::groupByIndex(uint32 index) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT_RUNTIME(index < impl->groups.size());
		return impl->groups[index];
	}

	uint32 entityManagerClass::componentsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	uint32 entityManagerClass::groupsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
	}

	groupClass *entityManagerClass::defineGroup()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		groupImpl *b = detail::systemArena().createObject<groupImpl>(impl);
		impl->groups.push_back(b);
		return b;
	}

	const groupClass *entityManagerClass::group() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return &impl->allEntities;
	}

	entityClass *entityManagerClass::createUnique()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return create(impl->generateUniqueName());
	}

	entityClass *entityManagerClass::createAnonymous()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return detail::systemArena().createObject<entityImpl>(impl, 0);
	}

	entityClass *entityManagerClass::create(uint32 name)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT_RUNTIME(name != 0);
		CAGE_ASSERT_RUNTIME(!impl->namedEntities->exists(name), "entity name must be unique", name);
		return detail::systemArena().createObject<entityImpl>(impl, name);
	}

	entityClass *entityManagerClass::get(uint32 entityName) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return impl->namedEntities->get(entityName, false);
	}

	entityClass *entityManagerClass::getOrCreate(uint32 entityName)
	{
		if (has(entityName))
			return get(entityName);
		return create(entityName);
	}

	bool entityManagerClass::has(uint32 entityName) const
	{
		if (entityName == 0)
			return false;
		return ((entityManagerImpl *)this)->namedEntities->exists(entityName);
	}

	void entityManagerClass::destroy()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		impl->allEntities.destroy();
	}

	componentClass *entityManagerClass::zPrivateDefineComponent(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const componentCreateConfig &config)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		componentImpl *c = detail::systemArena().createObject<componentImpl>(impl, typeSize, typeAlignment, prototype, config);
		impl->components.push_back(c);
		return c;
	}

	holder<entityManagerClass> newEntityManager(const entityManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<entityManagerClass, entityManagerImpl>(config);
	}

	uintPtr componentClass::typeSize() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->typeSize;
	}

	uint32 componentClass::index() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->vectorIndex;
	}

	const groupClass *componentClass::group() const
	{
		componentImpl *impl = (componentImpl *)this;
		return impl->componentEntities.get();
	}

	void componentClass::destroy()
	{
		componentImpl *impl = (componentImpl *)this;
		impl->componentEntities.get()->destroy();
	}

	entityManagerClass *componentClass::manager() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->manager;
	}

	uint32 entityClass::name() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->name;
	}

	void entityClass::add(groupClass *group)
	{
		if (has(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.insert(group);
		((groupImpl*)group)->entities.push_back(impl);
		group->entityAdded.dispatch(this);
	}

	void entityClass::remove(groupClass *group)
	{
		if (!has(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.erase(group);
		for (std::vector<entityClass*>::reverse_iterator it = ((groupImpl*)group)->entities.rbegin(), et = ((groupImpl*)group)->entities.rend(); it != et; it++)
		{
			if (*it == impl)
			{
				((groupImpl*)group)->entities.erase(--(it.base()));
				break;
			}
		}
		group->entityRemoved.dispatch(this);
	}

	bool entityClass::has(const groupClass *group) const
	{
		entityImpl *impl = (entityImpl*)this;
		CAGE_ASSERT_RUNTIME(((groupImpl*)group)->manager == impl->manager, "incompatible group");
		return impl->groups.find(const_cast<groupClass*>(group)) != impl->groups.end();
	}

	void entityClass::add(componentClass *component)
	{
		if (has(component))
			return;
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		if (impl->components.size() < ci->vectorIndex + 1)
			impl->components.resize(ci->vectorIndex + 1);
		void *c = detail::systemArena().allocate(ci->typeSize, ci->typeAlignment);
		impl->components[ci->vectorIndex] = c;
		detail::memcpy(c, ci->prototype, ci->typeSize);
		if (ci->componentEntities)
			ci->componentEntities->add(this);
	}

	void entityClass::remove(componentClass *component)
	{
		if (!has(component))
			return;
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		if (ci->componentEntities)
			ci->componentEntities->remove(this);
		detail::systemArena().deallocate(impl->components[ci->vectorIndex]);
		impl->components[ci->vectorIndex] = nullptr;
	}

	bool entityClass::has(const componentClass *component) const
	{
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		CAGE_ASSERT_RUNTIME(ci->manager == impl->manager, "incompatible component");
		if (impl->components.size() > ci->vectorIndex)
			return impl->components[ci->vectorIndex] != nullptr;
		return false;
	}

	void entityClass::destroy()
	{
		entityImpl *impl = (entityImpl*)this;
		detail::systemArena().destroy <entityImpl>(impl);
	}

	entityManagerClass *entityClass::manager() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->manager;
	}

	void *entityClass::unsafeValue(componentClass *component)
	{
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl*)component;
		if (impl->components.size() > ci->vectorIndex)
		{
			void *res = impl->components[ci->vectorIndex];
			if (res)
				return res;
		}
		add(component);
		return unsafeValue(component);
	}

	uint32 groupClass::index() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->vectorIndex;
	}

	uint32 groupClass::count() const
	{
		groupImpl *impl = (groupImpl*)this;
		return numeric_cast <uint32> (impl->entities.size());
	}

	entityClass *const *groupClass::array() const
	{
		groupImpl *impl = (groupImpl*)this;
		if (impl->entities.empty())
			return nullptr;
		return &impl->entities[0];
	}

	pointerRange<entityClass *const> groupClass::entities() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->entities;
	}

	void groupClass::merge(const groupClass *other)
	{
		if (other == this)
			return;
		for (auto it : other->entities())
			it->add(this);
	}

	void groupClass::subtract(const groupClass *other)
	{
		if (other == this)
		{
			clear();
			return;
		}
		for (auto it : other->entities())
			it->remove(this);
	}

	void groupClass::intersect(const groupClass *other)
	{
		if (other == this)
			return;
		std::vector<entityClass*> r;
		for (auto it : entities())
			if (!it->has(other))
				r.push_back(it);
		for (auto it : r)
			it->remove(this);
	}

	void groupClass::clear()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			impl->entities[0]->remove(this);
	}

	void groupClass::destroy()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			(*impl->entities.rbegin())->destroy();
	}

	entityManagerClass *groupClass::manager() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->manager;
	}

	memoryBuffer entitiesSerialize(const groupClass *entities, componentClass *component)
	{
		uintPtr typeSize = component->typeSize();
		memoryBuffer buffer;
		serializer ser(buffer);
		ser << component->index();
		ser << (uint64)typeSize;
		serializer cntPlaceholder = ser.placeholder(sizeof(uint32));
		uint32 cnt = 0;
		for (entityClass *e : entities->entities())
		{
			uint32 name = e->name();
			if (name == 0)
				continue;
			if (!e->has(component))
				continue;
			cnt++;
			ser << name;
			ser.write(e->unsafeValue(component), typeSize);
		}
		if (cnt == 0)
			return {};
		cntPlaceholder << cnt;
		return buffer;
	}

	void entitiesDeserialize(const void *buffer, uintPtr size, entityManagerClass *manager)
	{
		if (size == 0)
			return;
		deserializer des(buffer, size);
		uint32 componentIndex;
		des >> componentIndex;
		if (componentIndex >= manager->componentsCount())
			CAGE_THROW_ERROR(exception, "incompatible component (different index)");
		componentClass *component = manager->componentByIndex(componentIndex);
		uint64 typeSize;
		des >> typeSize;
		if (component->typeSize() != typeSize)
			CAGE_THROW_ERROR(exception, "incompatible component (different size)");
		uint32 cnt;
		des >> cnt;
		while (cnt--)
		{
			uint32 name;
			des >> name;
			if (name == 0)
				CAGE_THROW_ERROR(exception, "invalid entity");
			entityClass *e = manager->getOrCreate(name);
			des.read(e->unsafeValue(component), numeric_cast<uintPtr>(typeSize));
		}
	}

	void entitiesDeserialize(const memoryBuffer &buffer, entityManagerClass *manager)
	{
		entitiesDeserialize(buffer.data(), buffer.size(), manager);
	}
}
