#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/ctl/unordered_map.h>

#include <vector>
#include <set>

namespace cage
{
	namespace
	{
		class EntityManagerImpl;
		class ComponentImpl;
		class EntityImpl;
		class GroupImpl;

		class GroupImpl : public EntityGroup
		{
		public:
			EntityManagerImpl *const manager;
			std::vector<Entity*> entities;
			const uint32 vectorIndex;

			GroupImpl(EntityManagerImpl *manager);
		};

		class EntityManagerImpl : public EntityManager
		{
		public:
			std::vector<ComponentImpl*> components;
			std::vector<GroupImpl*> groups;
			GroupImpl allEntities;
			uint32 generateName;
			cage::unordered_map<uint32, Entity*> namedEntities;

#if defined(CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4355) // disable warning that using this in initializer list is dangerous
#endif

			EntityManagerImpl(const EntityManagerCreateConfig &config) : allEntities(this), generateName(0)
			{}

#if defined(CAGE_SYSTEM_WINDOWS)
#pragma warning (pop)
#endif

			~EntityManagerImpl()
			{
				allEntities.destroy();
				for (auto it = groups.begin(), et = groups.end(); it != et; it++)
					detail::systemArena().destroy<GroupImpl>(*it);
				for (auto it = components.begin(), et = components.end(); it != et; it++)
					detail::systemArena().destroy<ComponentImpl>(*it);
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

		GroupImpl::GroupImpl(EntityManagerImpl *manager) : manager(manager), vectorIndex(numeric_cast<uint32>(manager->groups.size()))
		{
			entities.reserve(100);
		}

		class ComponentImpl : public EntityComponent
		{
		public:
			EntityManagerImpl *const manager;
			Holder<GroupImpl> componentEntities;
			const uintPtr typeSize;
			const uintPtr typeAlignment;
			void *prototype;
			const uint32 vectorIndex;

			ComponentImpl(EntityManagerImpl *manager, uintPtr typeSize, uintPtr typeAlignment, void *prototype, const EntityComponentCreateConfig &config) :
				manager(manager), typeSize(typeSize), typeAlignment(typeAlignment), vectorIndex(numeric_cast<uint32>(manager->components.size()))
			{
				this->prototype = detail::systemArena().allocate(typeSize, typeAlignment);
				detail::memcpy(this->prototype, prototype, typeSize);
				if (config.enumerableEntities)
					componentEntities = detail::systemArena().createHolder<GroupImpl>(manager);
			}

			~ComponentImpl()
			{
				detail::systemArena().deallocate(prototype);
			}
		};

		class EntityImpl : public Entity
		{
		public:
			EntityManagerImpl *const manager;
			const uint32 name;
			std::set<EntityGroup*> groups;
			std::vector<void*> components;

			EntityImpl(EntityManagerImpl *manager, uint32 name) :
				manager(manager), name(name)
			{
				if (name != 0)
					manager->namedEntities.emplace(name, this);
				manager->allEntities.add(this);
			}

			~EntityImpl()
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
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		CAGE_ASSERT(index < impl->components.size());
		return impl->components[index];
	}

	EntityGroup *EntityManager::groupByIndex(uint32 index) const
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		CAGE_ASSERT(index < impl->groups.size());
		return impl->groups[index];
	}

	uint32 EntityManager::componentsCount() const
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	uint32 EntityManager::groupsCount() const
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
	}

	EntityGroup *EntityManager::defineGroup()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		GroupImpl *b = detail::systemArena().createObject<GroupImpl>(impl);
		impl->groups.push_back(b);
		return b;
	}

	const EntityGroup *EntityManager::group() const
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return &impl->allEntities;
	}

	Entity *EntityManager::createUnique()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return create(impl->generateUniqueName());
	}

	Entity *EntityManager::createAnonymous()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return detail::systemArena().createObject<EntityImpl>(impl, 0);
	}

	Entity *EntityManager::create(uint32 name)
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		CAGE_ASSERT(name != 0);
		CAGE_ASSERT(!impl->namedEntities.count(name), "entity name must be unique", name);
		return detail::systemArena().createObject<EntityImpl>(impl, name);
	}

	Entity *EntityManager::tryGet(uint32 entityName) const
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		auto it = impl->namedEntities.find(entityName);
		if (it == impl->namedEntities.end())
			return nullptr;
		return it->second;
	}

	Entity *EntityManager::get(uint32 entityName) const
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
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
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return impl->namedEntities.count(entityName);
	}

	void EntityManager::destroy()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		impl->allEntities.destroy();
	}

	EntityComponent *EntityManager::zPrivateDefineComponent(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const EntityComponentCreateConfig &config)
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		ComponentImpl *c = detail::systemArena().createObject<ComponentImpl>(impl, typeSize, typeAlignment, prototype, config);
		impl->components.push_back(c);
		return c;
	}

	Holder<EntityManager> newEntityManager(const EntityManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<EntityManager, EntityManagerImpl>(config);
	}

	uintPtr EntityComponent::typeSize() const
	{
		ComponentImpl *impl = (ComponentImpl*)this;
		return impl->typeSize;
	}

	uint32 EntityComponent::index() const
	{
		ComponentImpl *impl = (ComponentImpl*)this;
		return impl->vectorIndex;
	}

	const EntityGroup *EntityComponent::group() const
	{
		ComponentImpl *impl = (ComponentImpl *)this;
		return impl->componentEntities.get();
	}

	void EntityComponent::destroy()
	{
		ComponentImpl *impl = (ComponentImpl *)this;
		impl->componentEntities.get()->destroy();
	}

	EntityManager *EntityComponent::manager() const
	{
		ComponentImpl *impl = (ComponentImpl*)this;
		return impl->manager;
	}

	uint32 Entity::name() const
	{
		EntityImpl *impl = (EntityImpl*)this;
		return impl->name;
	}

	void Entity::add(EntityGroup *group)
	{
		if (has(group))
			return;
		EntityImpl *impl = (EntityImpl*)this;
		impl->groups.insert(group);
		((GroupImpl*)group)->entities.push_back(impl);
		group->entityAdded.dispatch(this);
	}

	void Entity::remove(EntityGroup *group)
	{
		if (!has(group))
			return;
		EntityImpl *impl = (EntityImpl*)this;
		impl->groups.erase(group);
		for (std::vector<Entity*>::reverse_iterator it = ((GroupImpl*)group)->entities.rbegin(), et = ((GroupImpl*)group)->entities.rend(); it != et; it++)
		{
			if (*it == impl)
			{
				((GroupImpl*)group)->entities.erase(--(it.base()));
				break;
			}
		}
		group->entityRemoved.dispatch(this);
	}

	bool Entity::has(const EntityGroup *group) const
	{
		EntityImpl *impl = (EntityImpl*)this;
		CAGE_ASSERT(((GroupImpl*)group)->manager == impl->manager, "incompatible group");
		return impl->groups.find(const_cast<EntityGroup*>(group)) != impl->groups.end();
	}

	void Entity::add(EntityComponent *component)
	{
		if (has(component))
			return;
		EntityImpl *impl = (EntityImpl*)this;
		ComponentImpl *ci = (ComponentImpl *)component;
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
		EntityImpl *impl = (EntityImpl*)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (ci->componentEntities)
			ci->componentEntities->remove(this);
		detail::systemArena().deallocate(impl->components[ci->vectorIndex]);
		impl->components[ci->vectorIndex] = nullptr;
	}

	bool Entity::has(const EntityComponent *component) const
	{
		EntityImpl *impl = (EntityImpl*)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		CAGE_ASSERT(ci->manager == impl->manager, "incompatible component");
		if (impl->components.size() > ci->vectorIndex)
			return impl->components[ci->vectorIndex] != nullptr;
		return false;
	}

	void Entity::destroy()
	{
		CAGE_ASSERT(this); // calling free/delete on null is ok, but calling the destroy METHOD is not, and some compilers totally ignored that issue
		EntityImpl *impl = (EntityImpl*)this;
		detail::systemArena().destroy<EntityImpl>(impl);
	}

	EntityManager *Entity::manager() const
	{
		EntityImpl *impl = (EntityImpl*)this;
		return impl->manager;
	}

	void *Entity::unsafeValue(EntityComponent *component)
	{
		EntityImpl *impl = (EntityImpl*)this;
		ComponentImpl *ci = (ComponentImpl*)component;
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
		GroupImpl *impl = (GroupImpl*)this;
		return impl->vectorIndex;
	}

	uint32 EntityGroup::count() const
	{
		GroupImpl *impl = (GroupImpl*)this;
		return numeric_cast <uint32> (impl->entities.size());
	}

	Entity *const *EntityGroup::array() const
	{
		GroupImpl *impl = (GroupImpl*)this;
		if (impl->entities.empty())
			return nullptr;
		return &impl->entities[0];
	}

	PointerRange<Entity *const> EntityGroup::entities() const
	{
		GroupImpl *impl = (GroupImpl*)this;
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
		GroupImpl *impl = (GroupImpl*)this;
		while (!impl->entities.empty())
			impl->entities[0]->remove(this);
	}

	void EntityGroup::destroy()
	{
		GroupImpl *impl = (GroupImpl*)this;
		while (!impl->entities.empty())
			(*impl->entities.rbegin())->destroy();
	}

	EntityManager *EntityGroup::manager() const
	{
		GroupImpl *impl = (GroupImpl*)this;
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
