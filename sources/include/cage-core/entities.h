#ifndef guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
#define guard_entities_h_1259B2E89D514872B54F01F42E1EC56A

#include "events.h"

namespace cage
{
	struct CAGE_CORE_API EntityComponentCreateConfig
	{
		bool enumerableEntities = false;

		EntityComponentCreateConfig(bool enumerableEntities) : enumerableEntities(enumerableEntities)
		{}
	};

	class CAGE_CORE_API EntityManager : private Immovable
	{
	public:
		template<class T>
		EntityComponent *defineComponent(const T &prototype, const EntityComponentCreateConfig &config)
		{
			static_assert(std::is_trivially_copyable<T>::value && std::is_trivially_destructible<T>::value, "type not trivial");
			return defineComponent_(sizeof(T), alignof(T), (void *)&prototype, config);
		}

		EntityComponent *componentByIndex(uint32 index) const;
		uint32 componentsCount() const;

		EntityGroup *defineGroup();
		EntityGroup *groupByIndex(uint32 index) const;
		uint32 groupsCount() const;
		const EntityGroup *group() const; // group containing all entities

		Entity *createUnique(); // new entity with unique name
		Entity *createAnonymous(); // new entity with name = 0, accessible only with the pointer
		Entity *create(uint32 name); // must be non-zero
		Entity *tryGet(uint32 entityName) const; // return nullptr if it does not exist
		Entity *get(uint32 entityName) const; // throw if it does not exist
		Entity *getOrCreate(uint32 entityName);
		bool has(uint32 entityName) const;
		PointerRange<Entity *const> entities() const;

		void destroy(); // destroy all entities

	private:
		EntityComponent *defineComponent_(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const EntityComponentCreateConfig &config);
	};

	struct CAGE_CORE_API EntityManagerCreateConfig
	{};

	CAGE_CORE_API Holder<EntityManager> newEntityManager(const EntityManagerCreateConfig &config);

	class CAGE_CORE_API Entity : private Immovable
	{
	public:
		uint32 name() const;
		EntityManager *manager() const;

		void add(EntityGroup *group);
		void remove(EntityGroup *group);
		bool has(const EntityGroup *group) const;

		void add(EntityComponent *component);
		template<class T> void add(EntityComponent *component, const T &prototype) { add(component); value<T>(component) = prototype; }
		void remove(EntityComponent *component);
		bool has(const EntityComponent *component) const;
		template<class T> T &value(EntityComponent *component);
		void *unsafeValue(EntityComponent *component);

		void destroy();
	};

	class CAGE_CORE_API EntityComponent : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 index() const;
		uintPtr typeSize() const;

		const EntityGroup *group() const; // all entities with this component
		PointerRange<Entity *const> entities() const;

		void destroy(); // destroy all entities with this component
	};

	class CAGE_CORE_API EntityGroup : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 index() const;

		uint32 count() const;
		Entity *const *array() const;
		PointerRange<Entity *const> entities() const;

		void add(Entity *ent) { ent->add(this); }
		void remove(Entity *ent) { ent->remove(this); }
		void add(uint32 entityName) { add(manager()->get(entityName)); }
		void remove(uint32 entityName) { remove(manager()->get(entityName)); }

		void merge(const EntityGroup *other); // add all entities from the other group into this group
		void subtract(const EntityGroup *other); // remove all entities, which are present in the other group, from this group
		void intersect(const EntityGroup *other); // remove all entities, which are NOT present in the other group, from this group

		void clear(); // remove all entities from this group
		void destroy(); // destroy all entities in this group

		mutable EventDispatcher<bool(Entity*)> entityAdded;
		mutable EventDispatcher<bool(Entity*)> entityRemoved;
	};

	inline PointerRange<Entity *const> EntityManager::entities() const { return group()->entities(); }
	template<class T> inline T &Entity::value(EntityComponent *component) { CAGE_ASSERT(component->manager() == manager()); CAGE_ASSERT(component->typeSize() == sizeof(T)); return *(T *)unsafeValue(component); }
	inline PointerRange<Entity *const> EntityComponent::entities() const { return group()->entities(); }

	CAGE_CORE_API Holder<PointerRange<char>> entitiesSerialize(const EntityGroup *entities, EntityComponent *component);
	CAGE_CORE_API void entitiesDeserialize(PointerRange<const char> buffer, EntityManager *manager);
}

#endif // guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
