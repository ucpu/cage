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
			const uint32 id = m;

			EntityImpl(EntityManagerImpl *manager, uint32 id);
			~EntityImpl();

			void *&comp(uint32 i) const;
		};

		class EntityManagerImpl : public EntityManager
		{
		public:
			const EntityManagerCreateConfig config;
			Holder<MemoryArena> arena;
			std::vector<Holder<ComponentImpl>> components;
			std::vector<EntityComponent *> componentsByTypes;
			ankerl::unordered_dense::map<uint32, Entity *> namedEntities;
			FlatBag<Entity *> allEntities;
			uint32 generateId = 0;
			uint32 entSize = 0;

			EntityManagerImpl(const EntityManagerCreateConfig &config) : config(config) {}

			~EntityManagerImpl()
			{
				purge();
				components.clear();
			}

			uint32 generateUniqueId()
			{
				static constexpr uint32 a = (uint32)1 << 28;
				static constexpr uint32 b = (uint32)1 << 30;
				if (generateId < a || generateId > b)
					generateId = a;
				while (exists(generateId))
					generateId = generateId == b ? a : generateId + 1;
				return generateId++;
			}

			EntityImpl *newEnt(uint32 id)
			{
				void *ptr = arena->allocate(entSize, alignof(EntityImpl));
				try
				{
					return new (ptr, privat::OperatorNewTrait()) EntityImpl(this, id);
				}
				catch (...)
				{
					if (!config.linearAllocators)
						arena->deallocate(ptr);
					throw;
				}
			}

			void desEnt(EntityImpl *e)
			{
				if (config.linearAllocators)
					e->~EntityImpl();
				else
					arena->destroy<EntityImpl>(e);
			}
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
				if (manager->config.linearAllocators)
					arena = newMemoryAllocatorLinear({});
				else
					arena = newMemoryAllocatorPool({ typeSize, typeAlignment });
			}

			void *newVal() { return arena->allocate(typeSize, typeAlignment); }

			void desVal(void *v)
			{
				if (!manager->config.linearAllocators)
					arena->deallocate(v);
			}
		};

		EntityImpl::EntityImpl(EntityManagerImpl *manager, uint32 id) : manager(manager), id(id)
		{
			for (uint32 i = 0; i < manager->components.size(); i++)
				comp(i) = nullptr;
			manager->allEntities.insert(this);
			if (id != 0)
				manager->namedEntities.emplace(id, this);
			manager->entityAdded.dispatch(this);
		}

		EntityImpl::~EntityImpl()
		{
			manager->entityRemoved.dispatch(this);
			if (id != 0)
				manager->namedEntities.erase(id);
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
		return create(impl->generateUniqueId());
	}

	Entity *EntityManager::createAnonymous()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		return impl->newEnt(0);
	}

	Entity *EntityManager::create(uint32 entityId)
	{
		CAGE_ASSERT(entityId != 0 && entityId != m);
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		CAGE_ASSERT(!impl->namedEntities.count(entityId));
		return impl->newEnt(entityId);
	}

	Entity *EntityManager::tryGet(uint32 entityId) const
	{
		CAGE_ASSERT(entityId != 0 && entityId != m);
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		auto it = impl->namedEntities.find(entityId);
		if (it == impl->namedEntities.end())
			return nullptr;
		return it->second;
	}

	Entity *EntityManager::get(uint32 entityId) const
	{
		CAGE_ASSERT(entityId != 0 && entityId != m);
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		try
		{
			return impl->namedEntities.at(entityId);
		}
		catch (const std::out_of_range &)
		{
			CAGE_LOG_THROW(Stringizer() + "id: " + entityId);
			CAGE_THROW_ERROR(Exception, "entity not found");
		}
	}

	Entity *EntityManager::getOrCreate(uint32 entityId)
	{
		CAGE_ASSERT(entityId != 0 && entityId != m);
		Entity *e = tryGet(entityId);
		if (e)
			return e;
		return create(entityId);
	}

	bool EntityManager::exists(uint32 entityId) const
	{
		CAGE_ASSERT(entityId != m);
		if (entityId == 0)
			return false;
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return impl->namedEntities.count(entityId);
	}

	PointerRange<Entity *const> EntityManager::entities() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return impl->allEntities;
	}

	void EntityManager::destroy()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		// first call all callbacks
		while (!impl->allEntities.empty())
			impl->allEntities.unsafeData().back()->destroy();
		// second free the memory
		impl->purge();
	}

	void EntityManager::purge()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		impl->namedEntities.clear();
		impl->allEntities.clear();
		if (impl->arena)
			impl->arena->flush();
		for (const auto &it : impl->components)
		{
			it->componentEntities.clear();
			it->arena->flush();
		}
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
		if (impl->config.linearAllocators)
			impl->arena = newMemoryAllocatorLinear({});
		else
			impl->arena = newMemoryAllocatorPool({ impl->entSize, alignof(EntityImpl) });
		return c;
	}

	EntityComponent *EntityManager::defineComponent(EntityComponent *source)
	{
		return defineComponent_(source->typeIndex(), +((ComponentImpl *)source)->prototype);
	}

	Holder<EntityManager> newEntityManager(const EntityManagerCreateConfig &config)
	{
		return systemMemory().createImpl<EntityManager, EntityManagerImpl>(config);
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

	uint32 Entity::id() const
	{
		const EntityImpl *impl = (const EntityImpl *)this;
		return impl->id;
	}

	void Entity::remove(EntityComponent *component)
	{
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		void *&ptr = impl->comp(ci->definitionIndex);
		if (ptr == nullptr)
			return;
		ci->componentEntities.erase(this);
		ci->desVal(ptr);
		ptr = nullptr;
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
		CAGE_ASSERT(component->manager() == manager());
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		void *&ptr = impl->comp(ci->definitionIndex);
		if (ptr == nullptr)
		{
			ptr = ci->newVal();
			detail::memcpy(ptr, +ci->prototype, ci->typeSize);
			ci->componentEntities.insert(this);
		}
		return ptr;
	}

	void Entity::destroy()
	{
		CAGE_ASSERT(this); // calling free/delete on null is ok, but calling the destroy METHOD is not, and some compilers totally ignored that issue
		EntityManagerImpl *man = (EntityManagerImpl *)(manager());
		man->desEnt((EntityImpl *)this);
	}

	namespace
	{
		struct ComponnentMapping
		{
			EntityComponent *sc = nullptr;
			EntityComponent *dc = nullptr;
		};

		std::vector<ComponnentMapping> componnentMapping(const EntitiesCopyConfig &config)
		{
			std::vector<ComponnentMapping> res;
			res.reserve(config.source->components().size());
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
				res.push_back({ sc, cbts[idx] });
			}
			return res;
		}
	}

	void entitiesCopy(const EntitiesCopyConfig &config)
	{
		const EntityManagerImpl *src = (const EntityManagerImpl *)config.source;
		EntityManagerImpl *dst = (EntityManagerImpl *)config.destination;

		if (config.purge)
			dst->purge();
		else
			dst->destroy();

		// must define components before changing the allocators
		const auto mp = componnentMapping(config);

		ankerl::unordered_dense::map<Entity *, EntityImpl *> ents; // old -> new
		ents.reserve(src->count());
		dst->allEntities.unsafeData().reserve(src->allEntities.size());
		for (Entity *se : src->allEntities)
		{
			EntityImpl *n = (EntityImpl *)dst->arena->allocate(dst->entSize, alignof(EntityImpl));
			detail::memset(n, 0, dst->entSize);
			const_cast<EntityManagerImpl *&>(n->manager) = dst;
			const_cast<uint32 &>(n->id) = se->id();
			dst->allEntities.unsafeData().push_back(n);
			ents[se] = n;
		}
		if (config.rebuildIndices)
			dst->allEntities.unsafeRebuildIndex();
		for (Entity *e : dst->allEntities)
		{
			if (e->id())
				dst->namedEntities.emplace(e->id(), e);
		}

		for (const auto &it : mp)
		{
			const auto sz = detail::typeSizeByIndex(it.sc->typeIndex());
			const uint32 si = it.sc->definitionIndex();
			const uint32 di = it.dc->definitionIndex();
			ComponentImpl *dc = ((ComponentImpl *)it.dc);
			for (EntityImpl *se : it.sc->entities().cast<EntityImpl *const>())
			{
				EntityImpl *de = ents[se];
				detail::memcpy(de->comp(di) = dc->newVal(), se->comp(si), sz);
				dc->componentEntities.unsafeData().push_back(de);
			}
			if (config.rebuildIndices)
				dc->componentEntities.unsafeRebuildIndex();
		}
	}
}
