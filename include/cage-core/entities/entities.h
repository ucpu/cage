namespace cage
{
	template struct CAGE_API delegate<void(componentClass*, entityClass*)>;
	template struct CAGE_API delegate<void(groupClass*, entityClass*)>;
	template struct CAGE_API delegate<void(entityClass *)>;
	template struct CAGE_API eventDispatcher<bool(entityClass*)>;

	class CAGE_API entityManagerClass
	{
	public:
		template<class T> componentClass *defineComponent(const T &prototype, bool enumerableEntities) { return zPrivateDefineComponent(sizeof(T), (void*)&prototype, enumerableEntities); }
		componentClass *getComponentByIndex(uint32 index);
		uint32 getComponentsCount() const;
		groupClass *defineGroup();
		groupClass *getGroupByIndex(uint32 index);
		uint32 getGroupsCount() const;

		entityClass *newEntity(uint32 name = 0);
		entityClass *getEntity(uint32 entityName);
		bool hasEntity(uint32 entityName) const;

		groupClass *getAllEntities(); // do not use it to add/remove entities directly

		uint32 generateUniqueName();

	private:
		componentClass *zPrivateDefineComponent(uintPtr typeSize, void *prototype, bool enumerableEntities);
	};

	struct CAGE_API entityManagerCreateConfig
	{};

	CAGE_API holder<entityManagerClass> newEntityManager(const entityManagerCreateConfig &config);

	class CAGE_API componentClass
	{
	public:
		entityManagerClass *getManager() const;
		uint32 getComponentIndex() const;
		uintPtr getTypeSize() const;
		groupClass *getComponentEntities(); // do not use it to add/remove entities directly
	};

	class CAGE_API entityClass
	{
	public:
		uint32 getName() const;
		entityManagerClass *getManager() const;

		void addGroup(groupClass *group);
		void removeGroup(groupClass *group);
		bool hasGroup(const groupClass *group) const;

		void addComponent(componentClass *component);
		template<class T> void addComponent(componentClass *component, const T &prototype) { addComponent(component); value<T>(component) = prototype; }
		void removeComponent(componentClass *component);
		bool hasComponent(const componentClass *component) const;
		template<class T> T &value(componentClass *component) { CAGE_ASSERT_RUNTIME(component->getTypeSize() == sizeof(T), "type is incompatible", component->getTypeSize(), sizeof(T)); return *(T*)zPrivateValue(component); }
		void *unsafeValue(componentClass *component) { return zPrivateValue(component); }

		void destroy();

	private:
		void *zPrivateValue(componentClass *component);
	};

	class CAGE_API groupClass
	{
	public:
		entityManagerClass *getManager() const;
		uint32 getGroupIndex() const;

		uint32 entitiesCount() const;
		entityClass *const *entitiesArray();
		pointerRange<entityClass *const> entities();
		void entitiesCallback(const delegate<void(entityClass *)> &callback);

		void addEntity(entityClass *entity) { entity->addGroup(this); }
		void removeEntity(entityClass *entity) { entity->removeGroup(this); }
		void addEntity(uint32 entityName) { addEntity(getManager()->getEntity(entityName)); }
		void removeEntity(uint32 entityName) { removeEntity(getManager()->getEntity(entityName)); }

		void addGroup(groupClass *group);
		void removeGroup(groupClass *group);

		void clear();
		void destroyAllEntities();

		eventDispatcher<bool(entityClass*)> entityAdded;
		eventDispatcher<bool(entityClass*)> entityRemoved;
	};
}