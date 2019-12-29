#include <vector>
#include <set>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/ctl/unordered_map.h>

namespace cage
{
	namespace
	{
		class entityManagerImpl;
		class componentImpl;
		class entityImpl;
		class groupImpl;

		class groupImpl : public EntityGroup
		{
		public:
			entityManagerImpl *const manager;
			std::vector<Entity*> entities;
			const uint32 vectorIndex;

			groupImpl(entityManagerImpl *manager);
		};

		class entityManagerImpl : public EntityManager
		{
		public:
			std::vector<componentImpl*> components;
			std::vector<groupImpl*> groups;
			groupImpl allEntities;
			uint32 generateName;
			cage::unordered_map<uint32, Entity*> namedEntities;

#if defined(CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4355) // disable warning that using this in initializer list is dangerous
#endif

			entityManagerImpl(const EntityManagerCreateConfig &config) : allEntities(this), generateName(0)
			{}

#if defined(CAGE_SYSTEM_WINDOWS)
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

		class componentImpl : public EntityComponent
		{
		public:
			entityManagerImpl *const manager;
			Holder<groupImpl> componentEntities;
			const uintPtr typeSize;
			const uintPtr typeAlignment;
			void *prototype;
			const uint32 vectorIndex;

			componentImpl(entityManagerImpl *manager, uintPtr typeSize, uintPtr typeAlignment, void *prototype, const EntityComponentCreateConfig &config) :
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

		class entityImpl : public Entity
		{
		public:
			entityManagerImpl *const manager;
			const uint32 name;
			std::set<EntityGroup*> groups;
			std::vector<void*> components;

			entityImpl(entityManagerImpl *manager, uint32 name) :
				manager(manager), name(name)
			{
				if (name != 0)
					manager->namedEntities.emplace(name, this);
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
					manager->namedEntities.erase(name);
			}
		};
	}

	EntityComponent *EntityManager::componentByIndex(uint32 index) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT(index < impl->components.size());
		return impl->components[index];
	}

	EntityGroup *EntityManager::groupByIndex(uint32 index) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT(index < impl->groups.size());
		return impl->groups[index];
	}

	uint32 EntityManager::componentsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	uint32 EntityManager::groupsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
	}

	EntityGroup *EntityManager::defineGroup()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		groupImpl *b = detail::systemArena().createObject<groupImpl>(impl);
		impl->groups.push_back(b);
		return b;
	}

	const EntityGroup *EntityManager::group() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return &impl->allEntities;
	}

	Entity *EntityManager::createUnique()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return create(impl->generateUniqueName());
	}

	Entity *EntityManager::createAnonymous()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return detail::systemArena().createObject<entityImpl>(impl, 0);
	}

	Entity *EntityManager::create(uint32 name)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT(name != 0);
		CAGE_ASSERT(!impl->namedEntities.count(name), "entity name must be unique", name);
		return detail::systemArena().createObject<entityImpl>(impl, name);
	}

	Entity *EntityManager::tryGet(uint32 entityName) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		auto it = impl->namedEntities.find(entityName);
		if (it == impl->namedEntities.end())
			return nullptr;
		return it->second;
	}

	Entity *EntityManager::get(uint32 entityName) const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return impl->namedEntities.at(entityName);
	}

	Entity *EntityManager::getOrCreate(uint32 entityName)
	{
		Entity *e = tryGet(entityName);
		if (e)
			return e;
		return create(entityName);
	}

	bool EntityManager::has(uint32 entityName) const
	{
		if (entityName == 0)
			return false;
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return impl->namedEntities.count(entityName);
	}

	void EntityManager::destroy()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		impl->allEntities.destroy();
	}

	EntityComponent *EntityManager::zPrivateDefineComponent(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const EntityComponentCreateConfig &config)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		componentImpl *c = detail::systemArena().createObject<componentImpl>(impl, typeSize, typeAlignment, prototype, config);
		impl->components.push_back(c);
		return c;
	}

	Holder<EntityManager> newEntityManager(const EntityManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<EntityManager, entityManagerImpl>(config);
	}

	uintPtr EntityComponent::typeSize() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->typeSize;
	}

	uint32 EntityComponent::index() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->vectorIndex;
	}

	const EntityGroup *EntityComponent::group() const
	{
		componentImpl *impl = (componentImpl *)this;
		return impl->componentEntities.get();
	}

	void EntityComponent::destroy()
	{
		componentImpl *impl = (componentImpl *)this;
		impl->componentEntities.get()->destroy();
	}

	EntityManager *EntityComponent::manager() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->manager;
	}

	uint32 Entity::name() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->name;
	}

	void Entity::add(EntityGroup *group)
	{
		if (has(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.insert(group);
		((groupImpl*)group)->entities.push_back(impl);
		group->entityAdded.dispatch(this);
	}

	void Entity::remove(EntityGroup *group)
	{
		if (!has(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.erase(group);
		for (std::vector<Entity*>::reverse_iterator it = ((groupImpl*)group)->entities.rbegin(), et = ((groupImpl*)group)->entities.rend(); it != et; it++)
		{
			if (*it == impl)
			{
				((groupImpl*)group)->entities.erase(--(it.base()));
				break;
			}
		}
		group->entityRemoved.dispatch(this);
	}

	bool Entity::has(const EntityGroup *group) const
	{
		entityImpl *impl = (entityImpl*)this;
		CAGE_ASSERT(((groupImpl*)group)->manager == impl->manager, "incompatible group");
		return impl->groups.find(const_cast<EntityGroup*>(group)) != impl->groups.end();
	}

	void Entity::add(EntityComponent *component)
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

	void Entity::remove(EntityComponent *component)
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

	bool Entity::has(const EntityComponent *component) const
	{
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		CAGE_ASSERT(ci->manager == impl->manager, "incompatible component");
		if (impl->components.size() > ci->vectorIndex)
			return impl->components[ci->vectorIndex] != nullptr;
		return false;
	}

	void Entity::destroy()
	{
		CAGE_ASSERT(this); // calling free/delete on null is ok, but calling the destroy METHOD is not, and some compilers totally ignored that issue
		entityImpl *impl = (entityImpl*)this;
		detail::systemArena().destroy<entityImpl>(impl);
	}

	EntityManager *Entity::manager() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->manager;
	}

	void *Entity::unsafeValue(EntityComponent *component)
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

	uint32 EntityGroup::index() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->vectorIndex;
	}

	uint32 EntityGroup::count() const
	{
		groupImpl *impl = (groupImpl*)this;
		return numeric_cast <uint32> (impl->entities.size());
	}

	Entity *const *EntityGroup::array() const
	{
		groupImpl *impl = (groupImpl*)this;
		if (impl->entities.empty())
			return nullptr;
		return &impl->entities[0];
	}

	PointerRange<Entity *const> EntityGroup::entities() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->entities;
	}

	void EntityGroup::merge(const EntityGroup *other)
	{
		if (other == this)
			return;
		for (auto it : other->entities())
			it->add(this);
	}

	void EntityGroup::subtract(const EntityGroup *other)
	{
		if (other == this)
		{
			clear();
			return;
		}
		for (auto it : other->entities())
			it->remove(this);
	}

	void EntityGroup::intersect(const EntityGroup *other)
	{
		if (other == this)
			return;
		std::vector<Entity*> r;
		for (auto it : entities())
			if (!it->has(other))
				r.push_back(it);
		for (auto it : r)
			it->remove(this);
	}

	void EntityGroup::clear()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			impl->entities[0]->remove(this);
	}

	void EntityGroup::destroy()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			(*impl->entities.rbegin())->destroy();
	}

	EntityManager *EntityGroup::manager() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->manager;
	}

	MemoryBuffer entitiesSerialize(const EntityGroup *entities, EntityComponent *component)
	{
		uintPtr typeSize = component->typeSize();
		MemoryBuffer buffer;
		Serializer ser(buffer);
		ser << component->index();
		ser << (uint64)typeSize;
		Serializer cntPlaceholder = ser.placeholder(sizeof(uint32));
		uint32 cnt = 0;
		for (Entity *e : entities->entities())
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

	void entitiesDeserialize(const void *buffer, uintPtr size, EntityManager *manager)
	{
		if (size == 0)
			return;
		Deserializer des(buffer, size);
		uint32 componentIndex;
		des >> componentIndex;
		if (componentIndex >= manager->componentsCount())
			CAGE_THROW_ERROR(Exception, "incompatible component (different index)");
		EntityComponent *component = manager->componentByIndex(componentIndex);
		uint64 typeSize;
		des >> typeSize;
		if (component->typeSize() != typeSize)
			CAGE_THROW_ERROR(Exception, "incompatible component (different size)");
		uint32 cnt;
		des >> cnt;
		while (cnt--)
		{
			uint32 name;
			des >> name;
			if (name == 0)
				CAGE_THROW_ERROR(Exception, "anonymous entity");
			Entity *e = manager->getOrCreate(name);
			des.read(e->unsafeValue(component), numeric_cast<uintPtr>(typeSize));
		}
	}

	void entitiesDeserialize(const MemoryBuffer &buffer, EntityManager *manager)
	{
		entitiesDeserialize(buffer.data(), buffer.size(), manager);
	}
}
