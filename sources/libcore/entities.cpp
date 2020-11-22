#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/flatSet.h>
#include <cage-core/math.h>
#include <cage-core/macros.h>

#include <robin_hood.h>
#include <plf_colony.h>

#include <vector>
#include <algorithm>

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
			std::vector<Entity *> entities;
			EntityManagerImpl *const manager = nullptr;
			const uint32 vectorIndex = m;

			GroupImpl(EntityManagerImpl *manager);
		};

		using GroupsSet = FlatSet<EntityGroup *>;

		class EntityImpl : public Entity
		{
		public:
			GroupsSet groups;
			std::vector<void *> components;
			EntityManagerImpl *const manager = nullptr;
			const uint32 name = m;

			EntityImpl(EntityManagerImpl *manager, uint32 name);
			~EntityImpl();
		};

		class EntityManagerImpl : public EntityManager
		{
		public:
			std::vector<Holder<GroupImpl>> groups;
			std::vector<Holder<ComponentImpl>> components;
			robin_hood::unordered_map<uint32, Entity *> namedEntities;
			plf::colony<EntityImpl> ents;
			GroupImpl allEntities;
			uint32 generateName = 0;

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4355) // disable warning that using this in initializer list is dangerous
#endif

			EntityManagerImpl(const EntityManagerCreateConfig &config) : allEntities(this)
			{}

#ifdef _MSC_VER
#pragma warning (pop)
#endif

			~EntityManagerImpl()
			{
				allEntities.destroy();
				components.clear();
				groups.clear();
			}

			uint32 generateUniqueName()
			{
				constexpr uint32 a = (uint32)1 << 28;
				constexpr uint32 b = (uint32)1 << 30;
				if (generateName < a || generateName > b)
					generateName = a;
				while (has(generateName))
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}

			EntityImpl *newEnt(uint32 name)
			{
				return &*ents.emplace(this, name);
			}

			void desEnt(EntityImpl *e)
			{
				ents.erase(ents.get_iterator_from_pointer(e));
			}
		};

		class Values
		{
		public:
			virtual ~Values() = default;
			virtual void *newVal() = 0;
			virtual void desVal(void *val) = 0;
		};

		template<uintPtr Size>
		class ValuesImpl : public Values
		{
		public:
			struct Value
			{
				char data[Size];
			};

			plf::colony<Value> data;

			void *newVal() override
			{
				return &*data.emplace();
			}

			void desVal(void *val) override
			{
				data.erase(data.get_iterator_from_pointer((Value *)val));
			}
		};

		class ValuesFallback : public Values
		{
		public:
			const uintPtr size = m;
			const uintPtr alignment = m;

			ValuesFallback(uintPtr size, uintPtr alignment) : size(size), alignment(alignment)
			{}

			void *newVal() override
			{
				return detail::systemArena().allocate(size, alignment);
			}

			void desVal(void *val) override
			{
				detail::systemArena().deallocate(val);
			}
		};

		Holder<Values> newValues(uintPtr size, uintPtr alignment)
		{
			const uintPtr s = max(size, alignment);
			if (s > 128)
				return detail::systemArena().createHolder<ValuesFallback>(size, alignment).cast<Values>();

#define GCHL_GENERATE(SIZE) if (s <= SIZE) return detail::systemArena().createHolder<ValuesImpl<SIZE>>().cast<Values>();
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, 4, 8, 12, 16, 20, 24, 28, 32, 48, 64, 96, 128));
#undef GCHL_GENERATE

			CAGE_THROW_CRITICAL(NotImplemented, "impossible entity component size or alignment");
		}

		class ComponentImpl : public EntityComponent
		{
		public:
			EntityManagerImpl *const manager = nullptr;
			Holder<GroupImpl> componentEntities;
			Holder<Values> values;
			const uintPtr typeSize = m;
			const uintPtr typeAlignment = m;
			void *prototype = nullptr;
			const uint32 vectorIndex = m;

			ComponentImpl(EntityManagerImpl *manager, uintPtr typeSize, uintPtr typeAlignment, void *prototype_, const EntityComponentCreateConfig &config) :
				manager(manager), typeSize(typeSize), typeAlignment(typeAlignment), vectorIndex(numeric_cast<uint32>(manager->components.size()))
			{
				if (config.enumerableEntities)
					componentEntities = detail::systemArena().createHolder<GroupImpl>(manager);
				values = newValues(typeSize, typeAlignment);
				prototype = values->newVal();
				detail::memcpy(prototype, prototype_, typeSize);
			}

			~ComponentImpl()
			{
				if (prototype)
					values->desVal(prototype);
				prototype = nullptr;
			}

			void *newVal()
			{
				void *v = values->newVal();
				detail::memcpy(v, prototype, typeSize);
				return v;
			}

			void desVal(void *v)
			{
				values->desVal(v);
			}
		};

		EntityImpl::EntityImpl(EntityManagerImpl *manager, uint32 name) : manager(manager), name(name)
		{
			if (name != 0)
				manager->namedEntities.emplace(name, this);
			manager->allEntities.add(this);
		}

		EntityImpl::~EntityImpl()
		{
			for (uint32 i = 0, e = numeric_cast<uint32>(components.size()); i != e; i++)
				if (components[i])
					remove(+manager->components[i]);
			while (!groups.empty())
				remove(*groups.begin());
			if (name != 0)
				manager->namedEntities.erase(name);
		}

		GroupImpl::GroupImpl(EntityManagerImpl *manager) : manager(manager), vectorIndex(numeric_cast<uint32>(manager->groups.size()))
		{
			entities.reserve(100);
		}
	}

	EntityComponent *EntityManager::componentByIndex(uint32 index) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		CAGE_ASSERT(index < impl->components.size());
		return +impl->components[index];
	}

	EntityGroup *EntityManager::groupByIndex(uint32 index) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		CAGE_ASSERT(index < impl->groups.size());
		return +impl->groups[index];
	}

	uint32 EntityManager::componentsCount() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	uint32 EntityManager::groupsCount() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
	}

	EntityGroup *EntityManager::defineGroup()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		Holder<GroupImpl> h = detail::systemArena().createHolder<GroupImpl>(impl);
		GroupImpl *g = +h;
		impl->groups.push_back(templates::move(h));
		return g;
	}

	const EntityGroup *EntityManager::group() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
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
		return impl->newEnt(0);
	}

	Entity *EntityManager::create(uint32 name)
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		CAGE_ASSERT(name != 0);
		CAGE_ASSERT(!impl->namedEntities.count(name));
		return impl->newEnt(name);
	}

	Entity *EntityManager::tryGet(uint32 entityName) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		auto it = impl->namedEntities.find(entityName);
		if (it == impl->namedEntities.end())
			return nullptr;
		return it->second;
	}

	Entity *EntityManager::get(uint32 entityName) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
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
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return impl->namedEntities.count(entityName);
	}

	void EntityManager::destroy()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		impl->allEntities.destroy();
	}

	EntityComponent *EntityManager::defineComponent_(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const EntityComponentCreateConfig &config)
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		Holder<ComponentImpl> h = detail::systemArena().createHolder<ComponentImpl>(impl, typeSize, typeAlignment, prototype, config);
		ComponentImpl *c = +h;
		impl->components.push_back(templates::move(h));
		return c;
	}

	Holder<EntityManager> newEntityManager(const EntityManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<EntityManager, EntityManagerImpl>(config);
	}

	uintPtr EntityComponent::typeSize() const
	{
		const ComponentImpl *impl = (const ComponentImpl*)this;
		return impl->typeSize;
	}

	uint32 EntityComponent::index() const
	{
		const ComponentImpl *impl = (const ComponentImpl*)this;
		return impl->vectorIndex;
	}

	const EntityGroup *EntityComponent::group() const
	{
		const ComponentImpl *impl = (const ComponentImpl *)this;
		return impl->componentEntities.get();
	}

	void EntityComponent::destroy()
	{
		ComponentImpl *impl = (ComponentImpl *)this;
		impl->componentEntities.get()->destroy();
	}

	EntityManager *EntityComponent::manager() const
	{
		const ComponentImpl *impl = (const ComponentImpl*)this;
		return impl->manager;
	}

	uint32 Entity::name() const
	{
		const EntityImpl *impl = (const EntityImpl*)this;
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
		const EntityImpl *impl = (const EntityImpl*)this;
		CAGE_ASSERT(((GroupImpl*)group)->manager == impl->manager);
		return impl->groups.count(const_cast<EntityGroup*>(group)) != 0;
	}

	void Entity::add(EntityComponent *component)
	{
		if (has(component))
			return;
		EntityImpl *impl = (EntityImpl*)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (impl->components.size() < ci->vectorIndex + 1)
			impl->components.resize(ci->vectorIndex + 1);
		impl->components[ci->vectorIndex] = ci->newVal();
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
		ci->values->desVal(impl->components[ci->vectorIndex]);
		impl->components[ci->vectorIndex] = nullptr;
	}

	bool Entity::has(const EntityComponent *component) const
	{
		const EntityImpl *impl = (const EntityImpl*)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		CAGE_ASSERT(ci->manager == impl->manager);
		if (impl->components.size() > ci->vectorIndex)
			return impl->components[ci->vectorIndex] != nullptr;
		return false;
	}

	void Entity::destroy()
	{
		CAGE_ASSERT(this); // calling free/delete on null is ok, but calling the destroy METHOD is not, and some compilers totally ignored that issue
		EntityManagerImpl *man = (EntityManagerImpl *)(manager());
		man->desEnt((EntityImpl *)this);
	}

	EntityManager *Entity::manager() const
	{
		const EntityImpl *impl = (const EntityImpl*)this;
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
		const GroupImpl *impl = (const GroupImpl*)this;
		return impl->vectorIndex;
	}

	uint32 EntityGroup::count() const
	{
		const GroupImpl *impl = (const GroupImpl*)this;
		return numeric_cast <uint32> (impl->entities.size());
	}

	Entity *const *EntityGroup::array() const
	{
		const GroupImpl *impl = (const GroupImpl*)this;
		if (impl->entities.empty())
			return nullptr;
		return &impl->entities[0];
	}

	PointerRange<Entity *const> EntityGroup::entities() const
	{
		const GroupImpl *impl = (const GroupImpl*)this;
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
		const GroupImpl *impl = (const GroupImpl*)this;
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
			const char *u = (const char *)e->unsafeValue(component);
			ser.write({ u, u + typeSize });
		}
		if (cnt == 0)
			return {};
		cntPlaceholder << cnt;
		return buffer;
	}

	void entitiesDeserialize(PointerRange<const char> buffer, EntityManager *manager)
	{
		if (buffer.empty())
			return;
		Deserializer des(buffer);
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
			char *u = (char *)e->unsafeValue(component);
			des.read({ u, u + typeSize });
		}
	}
}
