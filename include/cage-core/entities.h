#ifndef guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
#define guard_entities_h_1259B2E89D514872B54F01F42E1EC56A

namespace cage
{
	struct CAGE_API entityComponentCreateConfig
	{
		bool enumerableEntities;

		entityComponentCreateConfig(bool enumerableEntities) : enumerableEntities(enumerableEntities)
		{}
	};

	class CAGE_API entityManager : private immovable
	{
	public:
		template<class T> entityComponent *defineComponent(const T &prototype, const entityComponentCreateConfig &config) { return zPrivateDefineComponent(sizeof(T), alignof(T), (void*)&prototype, config); }
		entityComponent *componentByIndex(uint32 index) const;
		uint32 componentsCount() const;

		entityGroup *defineGroup();
		entityGroup *groupByIndex(uint32 index) const;
		uint32 groupsCount() const;
		const entityGroup *group() const; // group containing all entities

		entity *createUnique(); // new entity with unique name
		entity *createAnonymous(); // new entity with name = 0, accessible only with the pointer
		entity *create(uint32 name); // must be non-zero
		entity *get(uint32 entityName) const; // throw if it does not exist
		entity *getOrCreate(uint32 entityName);
		bool has(uint32 entityName) const;
		pointerRange<entity *const> entities() const;

		void destroy(); // destroy all entities

	private:
		entityComponent *zPrivateDefineComponent(uintPtr typeSize, uintPtr typeAlignment, void *prototype, const entityComponentCreateConfig &config);
	};

	struct CAGE_API entityManagerCreateConfig
	{};

	CAGE_API holder<entityManager> newEntityManager(const entityManagerCreateConfig &config);

	class CAGE_API entity : private immovable
	{
	public:
		uint32 name() const;
		entityManager *manager() const;

		void add(entityGroup *group);
		void remove(entityGroup *group);
		bool has(const entityGroup *group) const;

		void add(entityComponent *component);
		template<class T> void add(entityComponent *component, const T &prototype) { add(component); value<T>(component) = prototype; }
		void remove(entityComponent *component);
		bool has(const entityComponent *component) const;
		template<class T> T &value(entityComponent *component);
		void *unsafeValue(entityComponent *component);

		void destroy();
	};

	class CAGE_API entityComponent : private immovable
	{
	public:
		entityManager *manager() const;
		uint32 index() const;
		uintPtr typeSize() const;

		const entityGroup *group() const; // all entities with this component
		pointerRange<entity *const> entities() const;

		void destroy(); // destroy all entities with this component
	};

	class CAGE_API entityGroup : private immovable
	{
	public:
		entityManager *manager() const;
		uint32 index() const;

		uint32 count() const;
		entity *const *array() const;
		pointerRange<entity *const> entities() const;

		void add(entity *ent) { ent->add(this); }
		void remove(entity *ent) { ent->remove(this); }
		void add(uint32 entityName) { add(manager()->get(entityName)); }
		void remove(uint32 entityName) { remove(manager()->get(entityName)); }

		void merge(const entityGroup *other); // add all entities from the other group into this group
		void subtract(const entityGroup *other); // remove all entities, which are present in the other group, from this group
		void intersect(const entityGroup *other); // remove all entities, which are NOT present in the other group, from this group

		void clear(); // remove all entities from this group
		void destroy(); // destroy all entities in this group

		mutable eventDispatcher<bool(entity*)> entityAdded;
		mutable eventDispatcher<bool(entity*)> entityRemoved;
	};

	inline pointerRange<entity *const> entityManager::entities() const { return group()->entities(); }
	template<class T> inline T &entity::value(entityComponent *component) { CAGE_ASSERT(component->typeSize() == sizeof(T), "type is incompatible", component->typeSize(), sizeof(T)); return *(T*)unsafeValue(component); }
	inline pointerRange<entity *const> entityComponent::entities() const { return group()->entities(); }

	CAGE_API memoryBuffer entitiesSerialize(const entityGroup *entities, entityComponent *component);
	CAGE_API void entitiesDeserialize(const memoryBuffer &buffer, entityManager *manager);
	CAGE_API void entitiesDeserialize(const void *buffer, uintPtr size, entityManager *manager);
}

#endif // guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
