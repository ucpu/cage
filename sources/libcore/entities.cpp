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

		class groupImpl : public entityGroup
		{
		public:
			entityManagerImpl *const manager;
			std::vector<entity*> entities;
			const uint32 vectorIndex;

			groupImpl(entityManagerImpl *manager);
		};

		class entityManagerImpl : public entityManager
		{
		public:
			std::vector<componentImpl*> components;
			std::vector<groupImpl*> groups;
			groupImpl allEntities;
			uint32 generateName;
			holder<hashTable<entityImpl>> namedEntities;

#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4355) // disable warning that using this in initializer list is dangerous
#endif

			entityManagerImpl(const entityManagerCreateConfig &config) : allEntities(this), generateName(0)
			{
				namedEntities = newHashTable<entityImpl>({});
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

		class componentImpl : public entityComponent
		{
		public:
			entityManagerImpl *const manager;
			holder<groupImpl> componentEntities;
			const uintPtr typeSize;
			const uintPtr typeAlignment;
			void *prototype;
			const uint32 vectorIndex;

			componentImpl(entityManagerImpl *manager, uintPtr typeSize, uintPtr typeAlignment, void *prototype, const entityComponentCreateConfig &config) :
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

		class entityImpl : public entity
		{
		public:
			entityManagerImpl *const manager;
			const uint32 name;
			std::set<entityGroup*> groups;
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

	entityComponent *entityManager::componentByIndex(uint32 index) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT(index < impl->components.size());
		return impl->components[index];
	}

	entityGroup *entityManager::groupByIndex(uint32 index) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT(index < impl->groups.size());
		return impl->groups[index];
	}

	uint32 entityManager::componentsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	uint32 entityManager::groupsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
	}

	entityGroup *entityManager::defineGroup()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		groupImpl *b = detail::systemArena().createObject<groupImpl>(impl);
		impl->groups.push_back(b);
		return b;
	}

	const entityGroup *entityManager::group() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return &impl->allEntities;
	}

	entity *entityManager::createUnique()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return create(impl->generateUniqueName());
	}

	entity *entityManager::createAnonymous()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return detail::systemArena().createObject<entityImpl>(impl, 0);
	}

	entity *entityManager::create(uint32 name)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT(name != 0);
		CAGE_ASSERT(!impl->namedEntities->exists(name), "entity name must be unique", name);
		return detail::systemArena().createObject<entityImpl>(impl, name);
	}

	entity *entityManager::get(uint32 entityName) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return impl->namedEntities->get(entityName);
	}

	entity *entityManager::getOrCreate(uint32 entityName)
	{
		if (has(entityName))
			return get(entityName);
		return create(entityName);
	}

	bool entityManager::has(uint32 entityName) const
	{
		if (entityName == 0)
			return false;
		return ((entityManagerImpl *)this)->namedEntities->exists(entityName);
	}

	void entityManager::destroy()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		impl->allEntities.destroy();
	}

	entityComponent *entityManager::zPrivateDefineComponent(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const entityComponentCreateConfig &config)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		componentImpl *c = detail::systemArena().createObject<componentImpl>(impl, typeSize, typeAlignment, prototype, config);
		impl->components.push_back(c);
		return c;
	}

	holder<entityManager> newEntityManager(const entityManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<entityManager, entityManagerImpl>(config);
	}

	uintPtr entityComponent::typeSize() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->typeSize;
	}

	uint32 entityComponent::index() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->vectorIndex;
	}

	const entityGroup *entityComponent::group() const
	{
		componentImpl *impl = (componentImpl *)this;
		return impl->componentEntities.get();
	}

	void entityComponent::destroy()
	{
		componentImpl *impl = (componentImpl *)this;
		impl->componentEntities.get()->destroy();
	}

	entityManager *entityComponent::manager() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->manager;
	}

	uint32 entity::name() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->name;
	}

	void entity::add(entityGroup *group)
	{
		if (has(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.insert(group);
		((groupImpl*)group)->entities.push_back(impl);
		group->entityAdded.dispatch(this);
	}

	void entity::remove(entityGroup *group)
	{
		if (!has(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.erase(group);
		for (std::vector<entity*>::reverse_iterator it = ((groupImpl*)group)->entities.rbegin(), et = ((groupImpl*)group)->entities.rend(); it != et; it++)
		{
			if (*it == impl)
			{
				((groupImpl*)group)->entities.erase(--(it.base()));
				break;
			}
		}
		group->entityRemoved.dispatch(this);
	}

	bool entity::has(const entityGroup *group) const
	{
		entityImpl *impl = (entityImpl*)this;
		CAGE_ASSERT(((groupImpl*)group)->manager == impl->manager, "incompatible group");
		return impl->groups.find(const_cast<entityGroup*>(group)) != impl->groups.end();
	}

	void entity::add(entityComponent *component)
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

	void entity::remove(entityComponent *component)
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

	bool entity::has(const entityComponent *component) const
	{
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		CAGE_ASSERT(ci->manager == impl->manager, "incompatible component");
		if (impl->components.size() > ci->vectorIndex)
			return impl->components[ci->vectorIndex] != nullptr;
		return false;
	}

	void entity::destroy()
	{
		entityImpl *impl = (entityImpl*)this;
		detail::systemArena().destroy <entityImpl>(impl);
	}

	entityManager *entity::manager() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->manager;
	}

	void *entity::unsafeValue(entityComponent *component)
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

	uint32 entityGroup::index() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->vectorIndex;
	}

	uint32 entityGroup::count() const
	{
		groupImpl *impl = (groupImpl*)this;
		return numeric_cast <uint32> (impl->entities.size());
	}

	entity *const *entityGroup::array() const
	{
		groupImpl *impl = (groupImpl*)this;
		if (impl->entities.empty())
			return nullptr;
		return &impl->entities[0];
	}

	pointerRange<entity *const> entityGroup::entities() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->entities;
	}

	void entityGroup::merge(const entityGroup *other)
	{
		if (other == this)
			return;
		for (auto it : other->entities())
			it->add(this);
	}

	void entityGroup::subtract(const entityGroup *other)
	{
		if (other == this)
		{
			clear();
			return;
		}
		for (auto it : other->entities())
			it->remove(this);
	}

	void entityGroup::intersect(const entityGroup *other)
	{
		if (other == this)
			return;
		std::vector<entity*> r;
		for (auto it : entities())
			if (!it->has(other))
				r.push_back(it);
		for (auto it : r)
			it->remove(this);
	}

	void entityGroup::clear()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			impl->entities[0]->remove(this);
	}

	void entityGroup::destroy()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			(*impl->entities.rbegin())->destroy();
	}

	entityManager *entityGroup::manager() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->manager;
	}

	memoryBuffer entitiesSerialize(const entityGroup *entities, entityComponent *component)
	{
		uintPtr typeSize = component->typeSize();
		memoryBuffer buffer;
		serializer ser(buffer);
		ser << component->index();
		ser << (uint64)typeSize;
		serializer cntPlaceholder = ser.placeholder(sizeof(uint32));
		uint32 cnt = 0;
		for (entity *e : entities->entities())
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

	void entitiesDeserialize(const void *buffer, uintPtr size, entityManager *manager)
	{
		if (size == 0)
			return;
		deserializer des(buffer, size);
		uint32 componentIndex;
		des >> componentIndex;
		if (componentIndex >= manager->componentsCount())
			CAGE_THROW_ERROR(exception, "incompatible component (different index)");
		entityComponent *component = manager->componentByIndex(componentIndex);
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
			entity *e = manager->getOrCreate(name);
			des.read(e->unsafeValue(component), numeric_cast<uintPtr>(typeSize));
		}
	}

	void entitiesDeserialize(const memoryBuffer &buffer, entityManager *manager)
	{
		entitiesDeserialize(buffer.data(), buffer.size(), manager);
	}
}
