#include <algorithm>
#include <vector>

#include <plf_colony.h>
#include <unordered_dense.h>

#include <cage-core/entities.h>
#include <cage-core/flatSet.h>
#include <cage-core/math.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>

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
			const uint32 definitionIndex = m;

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
			std::vector<EntityComponent *> componentsByTypes;
			ankerl::unordered_dense::map<uint32, Entity *> namedEntities;
			plf::colony<EntityImpl> ents;
			GroupImpl allEntities;
			uint32 generateName = 0;

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4355) // disable warning that using this in initializer list is dangerous
#endif

			EntityManagerImpl() : allEntities(this) {}

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

			~EntityManagerImpl()
			{
				allEntities.destroy();
				components.clear();
				groups.clear();
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

			EntityImpl *newEnt(uint32 name) { return &*ents.emplace(this, name); }

			void desEnt(EntityImpl *e) { ents.erase(ents.get_iterator(e)); }
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
			struct alignas(Size) Value
			{
				char data[Size];
			};

			plf::colony<Value, MemoryAllocatorStd<Value>> data;

			void *newVal() override { return &*data.emplace(); }

			void desVal(void *val) override { data.erase(data.get_iterator((Value *)val)); }
		};

		class ValuesFallback : public Values
		{
		public:
			const uintPtr size = m;
			const uintPtr alignment = m;

			ValuesFallback(uintPtr size, uintPtr alignment) : size(size), alignment(alignment) {}

			void *newVal() override { return systemMemory().allocate(size, alignment); }

			void desVal(void *val) override { systemMemory().deallocate(val); }
		};

		Holder<Values> newValues(uintPtr size, uintPtr alignment)
		{
			const uintPtr s = max(size, alignment);
			if (s > 128)
				return systemMemory().createHolder<ValuesFallback>(size, alignment).cast<Values>();

#define GCHL_GENERATE(SIZE) \
	if (s <= SIZE) \
		return systemMemory().createHolder<ValuesImpl<SIZE>>().cast<Values>();
			GCHL_GENERATE(4);
			GCHL_GENERATE(8);
			GCHL_GENERATE(16);
			GCHL_GENERATE(32);
			GCHL_GENERATE(64);
			GCHL_GENERATE(128);
#undef GCHL_GENERATE

			CAGE_THROW_CRITICAL(NotImplemented, "impossible entity component size or alignment");
		}

		class ComponentImpl : public EntityComponent
		{
		public:
			Holder<GroupImpl> componentEntities;
			Holder<Values> values;
			EntityManagerImpl *const manager = nullptr;
			void *prototype = nullptr;
			const uint32 typeIndex = m;
			const uint32 typeSize = m;
			const uint32 definitionIndex = m;

			ComponentImpl(EntityManagerImpl *manager, uint32 typeIndex, const void *prototype_) : manager(manager), typeIndex(typeIndex), typeSize(detail::typeSizeByIndex(typeIndex)), definitionIndex(numeric_cast<uint32>(manager->components.size()))
			{
				CAGE_ASSERT(typeSize > 0);
				componentEntities = systemMemory().createHolder<GroupImpl>(manager);
				values = newValues(typeSize, detail::typeAlignmentByIndex(typeIndex));
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

			void desVal(void *v) { values->desVal(v); }
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

		GroupImpl::GroupImpl(EntityManagerImpl *manager) : manager(manager), definitionIndex(numeric_cast<uint32>(manager->groups.size()))
		{
			entities.reserve(100);
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

	EntityGroup *EntityManager::defineGroup()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		Holder<GroupImpl> h = systemMemory().createHolder<GroupImpl>(impl);
		GroupImpl *g = +h;
		impl->groups.push_back(std::move(h));
		return g;
	}

	EntityGroup *EntityManager::groupByDefinition(uint32 definitionIndex) const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		CAGE_ASSERT(definitionIndex < impl->groups.size());
		return +impl->groups[definitionIndex];
	}

	uint32 EntityManager::groupsCount() const
	{
		const EntityManagerImpl *impl = (const EntityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
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

	void EntityManager::destroy()
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
		impl->allEntities.destroy();
	}

	EntityComponent *EntityManager::defineComponent_(uint32 typeIndex, const void *prototype)
	{
		EntityManagerImpl *impl = (EntityManagerImpl *)this;
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
		return c;
	}

	EntityComponent *EntityManager::defineComponent(EntityComponent *source)
	{
		return defineComponent_(source->typeIndex(), ((ComponentImpl *)source)->prototype);
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

	void Entity::add(EntityGroup *group)
	{
		if (has(group))
			return;
		EntityImpl *impl = (EntityImpl *)this;
		impl->groups.insert(group);
		((GroupImpl *)group)->entities.push_back(impl);
		group->entityAdded.dispatch(this);
	}

	void Entity::remove(EntityGroup *group)
	{
		if (!has(group))
			return;
		EntityImpl *impl = (EntityImpl *)this;
		impl->groups.erase(group);
		for (std::vector<Entity *>::reverse_iterator it = ((GroupImpl *)group)->entities.rbegin(), et = ((GroupImpl *)group)->entities.rend(); it != et; it++)
		{
			if (*it == impl)
			{
				((GroupImpl *)group)->entities.erase(--(it.base()));
				break;
			}
		}
		group->entityRemoved.dispatch(this);
	}

	bool Entity::has(const EntityGroup *group) const
	{
		CAGE_ASSERT(group->manager() == this->manager());
		const EntityImpl *impl = (const EntityImpl *)this;
		return impl->groups.count(const_cast<EntityGroup *>(group)) != 0;
	}

	void Entity::add(EntityComponent *component)
	{
		if (has(component))
			return;
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (impl->components.size() < ci->definitionIndex + 1)
			impl->components.resize(ci->definitionIndex + 1);
		impl->components[ci->definitionIndex] = ci->newVal();
		if (ci->componentEntities)
			ci->componentEntities->add(this);
	}

	void Entity::remove(EntityComponent *component)
	{
		if (!has(component))
			return;
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (ci->componentEntities)
			ci->componentEntities->remove(this);
		ci->values->desVal(impl->components[ci->definitionIndex]);
		impl->components[ci->definitionIndex] = nullptr;
	}

	bool Entity::has(const EntityComponent *component) const
	{
		CAGE_ASSERT(component->manager() == this->manager());
		const EntityImpl *impl = (const EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (impl->components.size() > ci->definitionIndex)
			return impl->components[ci->definitionIndex] != nullptr;
		return false;
	}

	void *Entity::unsafeValue(EntityComponent *component)
	{
		CAGE_ASSERT(component->manager() == this->manager());
		EntityImpl *impl = (EntityImpl *)this;
		ComponentImpl *ci = (ComponentImpl *)component;
		if (impl->components.size() > ci->definitionIndex)
		{
			void *res = impl->components[ci->definitionIndex];
			if (res)
				return res;
		}
		add(component);
		return unsafeValue(component);
	}

	void Entity::destroy()
	{
		CAGE_ASSERT(this); // calling free/delete on null is ok, but calling the destroy METHOD is not, and some compilers totally ignored that issue
		EntityManagerImpl *man = (EntityManagerImpl *)(manager());
		man->desEnt((EntityImpl *)this);
	}

	EntityManager *EntityGroup::manager() const
	{
		const GroupImpl *impl = (const GroupImpl *)this;
		return impl->manager;
	}

	uint32 EntityGroup::definitionIndex() const
	{
		const GroupImpl *impl = (const GroupImpl *)this;
		return impl->definitionIndex;
	}

	PointerRange<Entity *const> EntityGroup::entities() const
	{
		const GroupImpl *impl = (const GroupImpl *)this;
		return impl->entities;
	}

	void EntityGroup::merge(const EntityGroup *other)
	{
		CAGE_ASSERT(this != other);
		CAGE_ASSERT(other->manager() == this->manager());
		for (auto it : other->entities())
			it->add(this);
	}

	void EntityGroup::subtract(const EntityGroup *other)
	{
		CAGE_ASSERT(this != other);
		CAGE_ASSERT(other->manager() == this->manager());
		for (auto it : other->entities())
			it->remove(this);
	}

	void EntityGroup::intersect(const EntityGroup *other)
	{
		CAGE_ASSERT(this != other);
		CAGE_ASSERT(other->manager() == this->manager());
		std::vector<Entity *> r;
		r.reserve(entities().size());
		for (auto it : entities())
			if (!it->has(other))
				r.push_back(it);
		for (auto it : r)
			it->remove(this);
	}

	void EntityGroup::clear()
	{
		GroupImpl *impl = (GroupImpl *)this;
		while (!impl->entities.empty())
			impl->entities[0]->remove(this);
	}

	void EntityGroup::destroy()
	{
		GroupImpl *impl = (GroupImpl *)this;
		while (!impl->entities.empty())
			(*impl->entities.rbegin())->destroy();
	}

	Holder<PointerRange<char>> entitiesExportBuffer(const EntityGroup *entities, EntityComponent *component)
	{
		CAGE_ASSERT(entities->manager() == component->manager());
		const uintPtr typeSize = detail::typeSizeByIndex(component->typeIndex());
		MemoryBuffer buffer;
		Serializer ser(buffer);
		ser << component->definitionIndex();
		ser << (uint64)typeSize;
		Serializer cntPlaceholder = ser.reserve(sizeof(uint32));
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
