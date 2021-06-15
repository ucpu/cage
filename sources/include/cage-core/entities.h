#ifndef guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
#define guard_entities_h_1259B2E89D514872B54F01F42E1EC56A

#include "events.h"
#include "typeIndex.h"

namespace cage
{
	class CAGE_CORE_API EntityManager : private Immovable
	{
	public:
		template<class T>
		EntityComponent *defineComponent(const T &prototype)
		{
			static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);
			return defineComponent_(detail::typeIndex<T>(), &prototype);
		}

		EntityComponent *componentByDefinition(uint32 index) const;
		EntityComponent *componentByType(uint32 index) const;
		template<class T> EntityComponent *component() const { return componentByType(detail::typeIndex<T>()); }
		uint32 componentsCount() const;

		EntityGroup *defineGroup();
		EntityGroup *groupByDefinition(uint32 index) const;
		uint32 groupsCount() const;
		const EntityGroup *group() const; // all entities in this manager

		Entity *createUnique(); // new entity with unique name
		Entity *createAnonymous(); // new entity with name = 0, accessible only with the pointer
		Entity *create(uint32 name); // must be non-zero
		Entity *tryGet(uint32 entityName) const; // return nullptr if it does not exist
		Entity *get(uint32 entityName) const; // throw if it does not exist
		Entity *getOrCreate(uint32 entityName);
		bool has(uint32 entityName) const;
		PointerRange<Entity *const> entities() const;
		uint32 count() const { return numeric_cast<uint32>(entities().size()); }

		void destroy(); // destroy all entities

	private:
		EntityComponent *defineComponent_(uint32 typeIndex, const void *prototype);
	};

	CAGE_CORE_API Holder<EntityManager> newEntityManager();

	class CAGE_CORE_API EntityComponent : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 definitionIndex() const;
		uint32 typeIndex() const;

		const EntityGroup *group() const; // all entities with this component
		PointerRange<Entity *const> entities() const;
		uint32 count() const { return numeric_cast<uint32>(entities().size()); }

		void destroy(); // destroy all entities with this component
	};

	class CAGE_CORE_API Entity : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 name() const;

		void add(EntityGroup *group);
		void remove(EntityGroup *group);
		bool has(const EntityGroup *group) const;

		void add(EntityComponent *component);
		template<class T> void add(EntityComponent *component, const T &data) { value<T>(component) = data; }
		template<class T> void add(const T &data) { add(component_<T>(), data); }

		void remove(EntityComponent *component);
		template<class T> void remove() { remove(component_<T>()); }

		bool has(const EntityComponent *component) const;
		template<class T> bool has() const { return has(component_<T>()); }

		template<class T> T &value(EntityComponent *component) { CAGE_ASSERT(component->manager() == manager()); CAGE_ASSERT(component->typeIndex() == detail::typeIndex<T>()); return *(T *)unsafeValue(component); }
		template<class T> T &value() { return value<T>(component_<T>()); }
		void *unsafeValue(EntityComponent *component);

		void destroy();

	private:
		template<class T> EntityComponent *component_() const { return manager()->component<T>(); }
	};

	class CAGE_CORE_API EntityGroup : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 definitionIndex() const;

		PointerRange<Entity *const> entities() const;
		uint32 count() const { return numeric_cast<uint32>(entities().size()); }

		void add(Entity *ent) { ent->add(this); }
		void remove(Entity *ent) { ent->remove(this); }
		void add(uint32 entityName) { add(manager()->get(entityName)); }
		void remove(uint32 entityName) { remove(manager()->get(entityName)); }

		void merge(const EntityGroup *other); // add all entities from the other group into this group
		void subtract(const EntityGroup *other); // remove all entities, which are present in the other group, from this group
		void intersect(const EntityGroup *other); // remove all entities, which are NOT present in the other group, from this group

		void clear(); // remove all entities from this group
		void destroy(); // destroy all entities in this group

		mutable EventDispatcher<bool(Entity *)> entityAdded;
		mutable EventDispatcher<bool(Entity *)> entityRemoved;
	};

	inline PointerRange<Entity *const> EntityManager::entities() const { return group()->entities(); }
	inline PointerRange<Entity *const> EntityComponent::entities() const { return group()->entities(); }

	CAGE_CORE_API Holder<PointerRange<char>> entitiesExportBuffer(const EntityGroup *entities, EntityComponent *component);
	CAGE_CORE_API void entitiesImportBuffer(PointerRange<const char> buffer, EntityManager *manager);
}

#endif // guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
