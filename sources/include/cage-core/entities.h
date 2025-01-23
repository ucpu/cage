#ifndef guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
#define guard_entities_h_1259B2E89D514872B54F01F42E1EC56A

#include <cage-core/events.h>
#include <cage-core/typeIndex.h>

namespace cage
{
	class EntityManager;
	class EntityComponent;
	class Entity;

	template<class T>
	concept ComponentConcept = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T> && std::is_same_v<std::remove_cvref_t<T>, T>;

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

		Entity *createUnique(); // new entity with unique id
		Entity *createAnonymous(); // new entity with id = 0, accessible only with the pointer
		Entity *create(uint32 entityId); // must be non-zero
		Entity *tryGet(uint32 entityId) const; // return nullptr if it does not exist
		Entity *get(uint32 entityId) const; // throw if it does not exist
		Entity *getOrCreate(uint32 entityId);
		bool exists(uint32 entityId) const;
		PointerRange<Entity *const> entities() const;
		CAGE_FORCE_INLINE uint32 count() const { return numeric_cast<uint32>(entities().size()); }

		void destroy(); // destroy all entities
		void purge(); // destroy all entities (without any callbacks)

		EventDispatcher<bool(Entity *)> entityAdded;
		EventDispatcher<bool(Entity *)> entityRemoved;

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

		PointerRange<Entity *const> entities() const;
		CAGE_FORCE_INLINE uint32 count() const { return numeric_cast<uint32>(entities().size()); }

		void destroy(); // destroy all entities with this component
	};

	class CAGE_CORE_API Entity : private Immovable
	{
	public:
		EntityManager *manager() const;
		uint32 id() const;

		CAGE_FORCE_INLINE void add(EntityComponent *component) { unsafeValue(component); }
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
		bool has(PointerRange<const EntityComponent *> components) const;
		template<ComponentConcept T>
		CAGE_FORCE_INLINE bool has() const
		{
			return has(component_<T>());
		}

		template<ComponentConcept T>
		CAGE_FORCE_INLINE T &value(EntityComponent *component)
		{
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
}

#endif // guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
