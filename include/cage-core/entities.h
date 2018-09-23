#ifndef guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
#define guard_entities_h_1259B2E89D514872B54F01F42E1EC56A

namespace cage
{
	class CAGE_API entityManagerClass
	{
	public:
		template<class T> componentClass *defineComponent(const T &prototype, bool enumerableEntities) { return zPrivateDefineComponent(sizeof(T), (void*)&prototype, enumerableEntities); }
		componentClass *componentByIndex(uint32 index) const;
		uint32 componentsCount() const;

		groupClass *defineGroup();
		groupClass *groupByIndex(uint32 index) const;
		uint32 groupsCount() const;
		const groupClass *group() const; // group containing all entities

		entityClass *createUnique(); // new entity with unique name
		entityClass *createAnonymous(); // new entity with name = 0, accessible only with the pointer
		entityClass *create(uint32 name); // must be non-zero
		entityClass *get(uint32 entityName) const; // throw if it does not exist
		entityClass *getOrCreate(uint32 entityName);
		bool has(uint32 entityName) const;
		inline pointerRange<entityClass *const> entities() const;

		void destroy(); // destroy all entities

	private:
		componentClass *zPrivateDefineComponent(uintPtr typeSize, void *prototype, bool enumerableEntities);
	};

	struct CAGE_API entityManagerCreateConfig
	{};

	CAGE_API holder<entityManagerClass> newEntityManager(const entityManagerCreateConfig &config);

	class CAGE_API entityClass
	{
	public:
		uint32 name() const;
		entityManagerClass *manager() const;

		void add(groupClass *group);
		void remove(groupClass *group);
		bool has(const groupClass *group) const;

		void add(componentClass *component);
		template<class T> void add(componentClass *component, const T &prototype) { add(component); value<T>(component) = prototype; }
		void remove(componentClass *component);
		bool has(const componentClass *component) const;
		template<class T> inline T &value(componentClass *component);
		void *unsafeValue(componentClass *component);

		void destroy();
	};

	class CAGE_API componentClass
	{
	public:
		entityManagerClass *manager() const;
		uint32 index() const;
		uintPtr typeSize() const;

		const groupClass *group() const; // all entities with this component
		inline pointerRange<entityClass *const> entities() const;

		void destroy(); // destroy all entities with this component
	};

	class CAGE_API groupClass
	{
	public:
		entityManagerClass *manager() const;
		uint32 index() const;

		uint32 count() const;
		entityClass *const *array() const;
		pointerRange<entityClass *const> entities() const;

		void add(entityClass *entity) { entity->add(this); }
		void remove(entityClass *entity) { entity->remove(this); }
		void add(uint32 entityName) { add(manager()->get(entityName)); }
		void remove(uint32 entityName) { remove(manager()->get(entityName)); }

		void merge(const groupClass *other); // add all entities from the other group into this group
		void subtract(const groupClass *other); // remove all entities, which are present in the other group, from this group
		void intersect(const groupClass *other); // remove all entities, which are NOT present in the other group, from this group

		void clear(); // remove all entities from this group
		void destroy(); // destroy all entities in this group

		mutable eventDispatcher<bool(entityClass*)> entityAdded;
		mutable eventDispatcher<bool(entityClass*)> entityRemoved;
	};

	inline pointerRange<entityClass *const> entityManagerClass::entities() const { return group()->entities(); }
	template<class T> inline T &entityClass::value(componentClass *component) { CAGE_ASSERT_RUNTIME(component->typeSize() == sizeof(T), "type is incompatible", component->typeSize(), sizeof(T)); return *(T*)unsafeValue(component); }
	inline pointerRange<entityClass *const> componentClass::entities() const { return group()->entities(); }

	CAGE_API memoryBuffer entitiesSerialize(const groupClass *entities, componentClass *component);
	CAGE_API void entitiesDeserialize(const memoryBuffer &buffer, entityManagerClass *manager);
	CAGE_API void entitiesDeserialize(const void *buffer, uintPtr size, entityManagerClass *manager);
}

#endif // guard_entities_h_1259B2E89D514872B54F01F42E1EC56A
