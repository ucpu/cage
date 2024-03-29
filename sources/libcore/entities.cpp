#include <algorithm>
#include <vector>

#include <unordered_dense.h>

#include <cage-core/entities.h>
#include <cage-core/flatBag.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/pointerRangeHolder.h>

namespace cage
{
	namespace
	{
		class EntityManagerImpl;
		class ComponentImpl;
		class EntityImpl;

		class EntityImpl : public Entity
		{
		public:
			EntityManagerImpl *const manager = nullptr;
			const uint32 name = m;

			EntityImpl(EntityManagerImpl *manager, uint32 name);
			~EntityImpl();

			void *&comp(uint32 i) const;
		};

		class EntityManagerImpl : public EntityManager
		{
		public:
			Holder<MemoryArena> arena;
			std::vector<Holder<ComponentImpl>> components;
			std::vector<EntityComponent *> componentsByTypes;
			ankerl::unordered_dense::map<uint32, Entity *> namedEntities;
			FlatBag<Entity *> allEntities;
			uint32 generateName = 0;
			uint32 entSize = 0;

			~EntityManagerImpl()
			{
				destroy();
				components.clear();
			}

			uint32 generateUniqueName()
			{
				static constexpr uint32 a = (uint32)1 << 28;
				static constexpr uint32 b = (uint32)1 << 30;
				if (generateName < a || generateName > b)
					generateName = a;
				while (has(generateName))
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}

			EntityImpl *newEnt(uint32 name)
			{
				void *ptr = arena->allocate(entSize, alignof(EntityImpl));
				try
				{
					return new (ptr, privat::OperatorNewTrait()) EntityImpl(this, name);
				}
				catch (...)
				{
					arena->deallocate(ptr);
					throw;
				}
			}

			void desEnt(EntityImpl *e) { arena->destroy<EntityImpl>(e); }
		};

		class ComponentImpl : public EntityComponent
		{
		public:
			FlatBag<Entity *> componentEntities;
			EntityManagerImpl *const manager = nullptr;
			Holder<MemoryArena> arena;
			Holder<void> prototype;
			const uint32 typeIndex = m;
			const uint32 typeSize = m;
			const uint32 typeAlignment = m;
			const uint32 definitionIndex = m;

			ComponentImpl(EntityManagerImpl *manager, uint32 typeIndex, const void *prototype_) : manager(manager), typeIndex(typeIndex), typeSize(detail::typeSizeByIndex(typeIndex)), typeAlignment(detail::typeAlignmentByIndex(typeIndex)), definitionIndex(numeric_cast<uint32>(manager->components.size()))
			{
				CAGE_ASSERT(typeSize > 0);
				prototype = systemMemory().createBuffer(typeSize, typeAlignment).cast<void>();
				detail::memcpy(+prototype, prototype_, typeSize);
				arena = newMemoryAllocatorPool({ std::max(typeSize, uint32(sizeof(uintPtr))), typeAlignment });
			}

			void *newVal() { return arena->allocate(typeSize, typeAlignment); }

			void desVal(void *v) { arena->deallocate(v); }
		};

		EntityImpl::EntityImpl(EntityManagerImpl *manager, uint32 name) : manager(manager), name(name)
		{
			for (uint32 i = 0; i < manager->components.size(); i++)
				comp(i) = nullptr;
			manager->allEntities.insert(this);
			if (name != 0)
				manager->namedEntities.emplace(name, this);
			manager->entityAdded.dispatch(this);
		}

		EntityImpl::~EntityImpl()
		{
			manager->entityRemoved.dispatch(this);
			if (name != 0)
				manager->namedEntities.erase(name);
			manager->allEntities.erase(this);
			for (uint32 i = 0; i < manager->components.size(); i++)
				if (comp(i))
					remove(+manager->components[i]);
		}

		CAGE_FORCE_INLINE void *&EntityImpl::comp(uint32 i) const
		{
			CAGE_ASSERT(i < manager->components.size());
			void **arr = (void **)(((char *)this) + sizeof(EntityImpl));
			return arr[i];
		}
	}

	EntityComponent *EntityManager::componentByDefinition(uint32 definitionIndex) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		CAGE_ASSERT(definitionIndex < impl->components.size());
		return +impl->components[definitionIndex];
	}

	EntityComponent *EntityManager::componentByType(uint32 typeIndex) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		if (typeIndex < impl->componentsByTypes.size())
			return impl->componentsByTypes[typeIndex];
		return nullptr;
	}

	Holder<PointerRange<EntityComponent *>> EntityManager::componentsByType(uint32 typeIndex) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		PointerRangeHolder<EntityComponent *> res;
		res.reserve(impl->components.size());
		for (const auto &c : impl->components)
		{
			if (c->typeIndex == typeIndex)
				res.push_back(+c);
		}
		return res;
	}

	Holder<PointerRange<EntityComponent *>> EntityManager::components() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		PointerRangeHolder<EntityComponent *> res;
		res.reserve(impl->components.size());
		for (const auto &c : impl->components)
			res.push_back(+c);
		return res;
	}

	uint32 EntityManager::componentsCount() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	Entity *EntityManager::createUnique()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return create(impl->generateUniqueName());
	}

	Entity *EntityManager::createAnonymous()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return impl->newEnt(0);
	}

	Entity *EntityManager::create(uint32 entityName)
	{
		CAGE_ASSERT(entityName != 0 && entityName != m);
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		CAGE_ASSERT(!impl->namedEntities.count(entityName));
		return impl->newEnt(entityName);
	}

	Entity *EntityManager::tryGet(uint32 entityName) const
	{
		CAGE_ASSERT(entityName != 0 && entityName != m);
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		auto it = impl->namedEntities.find(entityName);
		if (it == impl->namedEntities.end())
			return nullptr;
		return it->second;
	}

	Entity *EntityManager::get(uint32 entityName) const
	{
		CAGE_ASSERT(entityName != 0 && entityName != m);
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		try
		{
			return impl->namedEntities.at(entityName);
		}
		catch (const std::out_of_range &)
		{
			CAGE_LOG_THROW(Stringizer() + "name: " + entityName);
			CAGE_THROW_ERROR(Exception, "entity not found");
		}
	}

	Entity *EntityManager::getOrCreate(uint32 entityName)
	{
		CAGE_ASSERT(entityName != 0 && entityName != m);
		Entity *e = tryGet(entityName);
		if (e)
			return e;
		return create(entityName);
	}

	bool EntityManager::has(uint32 entityName) const
	{
		CAGE_ASSERT(entityName != m);
		if (entityName == 0)
			return false;
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return impl->namedEntities.count(entityName);
	}

	PointerRange<Entity *const> EntityManager::entities() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return impl->allEntities;
	}

	void EntityManager::destroy()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		while (!impl->allEntities.empty())
			impl->allEntities.unsafeData().back()->destroy();
	}

	EntityComponent *EntityManager::defineComponent_(uint32 typeIndex, const void *prototype)
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		if (impl->count())
		{
			CAGE_THROW_CRITICAL(Exception, "cannot define new component with already existing entities");
		}
		Holder<ComponentImpl> h = systemMemory().createHolder<ComponentImpl>(impl, typeIndex, prototype);
		{
			auto &v = impl->componentsByTypes;
			if (v.size() <= typeIndex)
				v.resize(typeIndex + 1, nullptr);
			if (v[typeIndex] == nullptr)
				v[typeIndex] = +h;
		}
		ComponentImpl *c = +h;
		impl->components.push_back(std::move(h));
		impl->entSize = sizeof(EntityImpl) + impl->components.size() * sizeof(void *);
		impl->arena = newMemoryAllocatorPool({ impl->entSize, alignof(EntityImpl) });
		return c;
	}

	EntityComponent *EntityManager::defineComponent(EntityComponent *source)
	{
		return defineComponent_(source->typeIndex(), +((ComponentImpl *)source)->prototype);
	}

	Holder<EntityManager> newEntityManager()
	{
		return systemMemory().createImpl<EntityManager, EntityManagerImpl>();
	}

	EntityManager *EntityComponent::manager() const
	{
		const ComponentImpl *impl = (const ComponentImpl *)this;
		return impl->manager;
	}

	uint32 EntityComponent::definitionIndex() const
	{
		const ComponentImpl *impl = (const ComponentImpl *)this;
		return impl->definitionIndex;
	}

	uint32 EntityComponent::typeIndex() const
	{
		const ComponentImpl *impl = (const ComponentImpl *)this;
		return impl->typeIndex;
	}

	PointerRange<Entity *const> EntityComponent::entities() const
	{
		const ComponentImpl *impl = (const ComponentImpl *)this;
		return impl->componentEntities;
	}

	void EntityComponent::destroy()
	{
		ComponentImpl *impl = (ComponentImpl *)this;
		while (!impl->componentEntities.empty())
			impl->componentEntities.unsafeData().back()->destroy();
	}

	EntityManager *Entity::manager() const
	{
		const EntityImpl *impl = (const EntityImpl *)this;
		return impl->manager;
	}

	uint32 Entity::name() const
	{
		const EntityImpl *impl = (const EntityImpl *)this;
		return impl->name;
	}

	void Entity::remove(EntityComponent *component)
	{
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (impl->comp(ci->definitionIndex) == nullptr)
			return;
		ci->entityRemoved.dispatch(this);
		ci->componentEntities.erase(this);
		ci->desVal(impl->comp(ci->definitionIndex));
		impl->comp(ci->definitionIndex) = nullptr;
	}

	bool Entity::has(const EntityComponent *component) const
	{
		CAGE_ASSERT(component->manager() == this->manager());
		const EntityImpl *impl = (const EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		return impl->comp(ci->definitionIndex) != nullptr;
	}

	bool Entity::has(PointerRange<const EntityComponent *> components) const
	{
		const EntityImpl *impl = (const EntityImpl *)this;
		for (const ComponentImpl *ci : components.cast<const ComponentImpl *>())
		{
			CAGE_ASSERT(ci->manager == impl->manager);
			if (impl->comp(ci->definitionIndex) == nullptr)
				return false;
		}
		return true;
	}

	void *Entity::unsafeValue(EntityComponent *component)
	{
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		void *&c = impl->comp(ci->definitionIndex);
		if (c == nullptr)
		{
			c = ci->newVal();
			detail::memcpy(c, +ci->prototype, ci->typeSize);
			ci->componentEntities.insert(this);
			ci->entityAdded.dispatch(this);
		}
		return c;
	}

	void Entity::destroy()
	{
		CAGE_ASSERT(this); // calling free/delete on null is ok, but calling the destroy METHOD is not, and some compilers totally ignored that issue
		EntityManagerImpl *man = (EntityManagerImpl *)(manager());
		man->desEnt((EntityImpl *)this);
	}
}
