#ifndef guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
#define guard_entities_h_1259B2E89D514872B54F01F42E1EC56A

#include <cage-core/events.h>
#include <cage-core/typeIndex.h>

namespace cage
{
	class EntityManager;
	class EntityComponent;
	class Entity;
	class EntityGroup;

	template<class T>
	concept ComponentConcept = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

	class CAGE_CORE_API EntityManager : private Immovable
	{
	public:
		template<ComponentConcept T>
		EntityComponent *defineComponent(const T &prototype)
		{
			return defineComponent_(detail::typeIndex<T>(), &prototype);
		}
		EntityComponent *defineComponent(EntityComponent *source);

		EntityComponent *componentByDefinition(uint32 definitionIndex) const;
		EntityComponent *componentByType(uint32 typeIndex) const;
		Holder<PointerRange<EntityComponent *>> componentsByType(uint32 typeIndex) const;
		template<ComponentConcept T>
		CAGE_FORCE_INLINE EntityComponent *component() const
		{
			return componentByType(detail::typeIndex<T>());
		}
		Holder<PointerRange<EntityComponent *>> components() const;
		uint32 componentsCount() const;

		EntityGroup *defineGroup();
		EntityGroup *groupByDefinition(uint32 definitionIndex) const;
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
		CAGE_FORCE_INLINE uint32 count() const { return numeric_cast<uint32>(entities().size()); }

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
		CAGE_FORCE_INLINE uint32 count() const { return numeric_cast<uint32>(entities().size()); }

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
		template<ComponentConcept T>
		CAGE_FORCE_INLINE void add(EntityComponent *component, const T &data)
		{
			value<T>(component) = data;
		}
		template<ComponentConcept T>
		CAGE_FORCE_INLINE void add(const T &data)
		{
			add(component_<T>(), data);
		}

		void remove(EntityComponent *component);
		template<ComponentConcept T>
		CAGE_FORCE_INLINE void remove()
		{
			remove(component_<T>());
		}

		bool has(const EntityComponent *component) const;
		template<ComponentConcept T>
		CAGE_FORCE_INLINE bool has() const
		{
			return has(component_<T>());
		}

		template<ComponentConcept T>
		CAGE_FORCE_INLINE T &value(EntityComponent *component)
		{
			CAGE_ASSERT(component->manager() == manager());
			CAGE_ASSERT(component->typeIndex() == detail::typeIndex<T>());
			return *(T *)unsafeValue(component);
		}
		template<ComponentConcept T>
		CAGE_FORCE_INLINE T &value()
		{
			return value<T>(component_<T>());
		}
		void *unsafeValue(EntityComponent *component);

		void destroy();

	private:
		template<ComponentConcept T>
		CAGE_FORCE_INLINE EntityComponent *component_() const
		{
			return manager()->component<T>();
		}
	};

	class CAGE_CORE_API EntityGroup : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 definitionIndex() const;

		PointerRange<Entity *const> entities() const;
		CAGE_FORCE_INLINE uint32 count() const { return numeric_cast<uint32>(entities().size()); }

		CAGE_FORCE_INLINE void add(Entity *ent) { ent->add(this); }
		CAGE_FORCE_INLINE void remove(Entity *ent) { ent->remove(this); }
		CAGE_FORCE_INLINE void add(uint32 entityName) { add(manager()->get(entityName)); }
		CAGE_FORCE_INLINE void remove(uint32 entityName) { remove(manager()->get(entityName)); }

		void merge(const EntityGroup *other); // add all entities from the other group into this group
		void subtract(const EntityGroup *other); // remove all entities, which are present in the other group, from this group
		void intersect(const EntityGroup *other); // remove all entities, which are NOT present in the other group, from this group

		void clear(); // remove all entities from this group
		void destroy(); // destroy all entities in this group

		mutable EventDispatcher<bool(Entity *)> entityAdded;
		mutable EventDispatcher<bool(Entity *)> entityRemoved;
	};

	CAGE_FORCE_INLINE PointerRange<Entity *const> EntityManager::entities() const
	{
		return group()->entities();
	}
	CAGE_FORCE_INLINE PointerRange<Entity *const> EntityComponent::entities() const
	{
		return group()->entities();
	}

	CAGE_CORE_API Holder<PointerRange<char>> entitiesExportBuffer(const EntityGroup *entities, EntityComponent *component);
	CAGE_CORE_API void entitiesImportBuffer(PointerRange<const char> buffer, EntityManager *manager);
}

#endif // guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
