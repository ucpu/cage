#include <vector>
#include <set>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/utility/hashTable.h>

namespace cage
{
	namespace
	{
		class entityManagerImpl;
		class componentImpl;
		class entityImpl;
		class groupImpl;

		class groupImpl : public groupClass
		{
		public:
			entityManagerImpl *const manager;
			std::vector<entityClass*> entities;
			const uint32 vectorIndex;

			groupImpl(entityManagerImpl *manager);
		};

		class entityManagerImpl : public entityManagerClass
		{
		public:
			std::vector<componentImpl*> components;
			std::vector<groupImpl*> groups;
			groupImpl allEntities;
			uint32 generateName;
			holder<hashTableClass<entityImpl>> namedEntities;

#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4355) // disable warning that using this in initializer list is dangerous
#endif

			entityManagerImpl(const entityManagerCreateConfig &config) : allEntities(this), generateName(0)
			{
				namedEntities = newHashTable<entityImpl>(1024, 1024 * 1024);
			}

#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (pop)
#endif

			~entityManagerImpl()
			{
				getAllEntities()->destroyAllEntities();
				for (auto it = groups.begin(), et = groups.end(); it != et; it++)
					detail::systemArena().destroy<groupImpl>(*it);
				for (auto it = components.begin(), et = components.end(); it != et; it++)
					detail::systemArena().destroy<componentImpl>(*it);
			}
		};

		groupImpl::groupImpl(entityManagerImpl *manager) : manager(manager), vectorIndex(numeric_cast<uint32>(manager->groups.size()))
		{
			entities.reserve(100);
		}

		class componentImpl : public componentClass
		{
		public:
			entityManagerImpl *const manager;
			holder<groupImpl> componentEntities;
			const uintPtr typeSize;
			void *prototype;
			const uint32 vectorIndex;

			componentImpl(entityManagerImpl *manager, uintPtr typeSize, void *prototype, bool enumerableEntities) :
				manager(manager), typeSize(typeSize), vectorIndex(numeric_cast<uint32>(manager->components.size()))
			{
				this->prototype = detail::systemArena().allocate(typeSize);
				detail::memcpy(this->prototype, prototype, typeSize);
				if (enumerableEntities)
					componentEntities = detail::systemArena().createHolder<groupImpl>(manager);
			}

			~componentImpl()
			{
				detail::systemArena().deallocate(prototype);
			}
		};

		class entityImpl : public entityClass
		{
		public:
			entityManagerImpl *const manager;
			const uint32 name;
			std::set<groupClass*> groups;
			std::vector<void*> components;

			entityImpl(entityManagerImpl *manager, uint32 name) :
				manager(manager), name(name)
			{
				if (name != 0)
					manager->namedEntities->add(name, this);
				manager->allEntities.addEntity(this);
			}

			~entityImpl()
			{
				for (uint32 i = 0, e = numeric_cast<uint32>(components.size()); i != e; i++)
					if (components[i])
						removeComponent(manager->components[i]);
				while (!groups.empty())
					removeGroup(*groups.begin());
				if (name != 0)
					manager->namedEntities->remove(name);
			}
		};
	}

	componentClass *entityManagerClass::getComponentByIndex(uint32 index)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT_RUNTIME(index < impl->components.size());
		return impl->components[index];
	}

	groupClass *entityManagerClass::getGroupByIndex(uint32 index)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT_RUNTIME(index < impl->groups.size());
		return impl->groups[index];
	}

	uint32 entityManagerClass::getComponentsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->components.size());
	}

	uint32 entityManagerClass::getGroupsCount() const
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return numeric_cast<uint32>(impl->groups.size());
	}

	groupClass *entityManagerClass::defineGroup()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		groupImpl *b = detail::systemArena().createObject<groupImpl>(impl);
		impl->groups.push_back(b);
		return b;
	}

	entityClass *entityManagerClass::newEntity(uint32 name)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		CAGE_ASSERT_RUNTIME(name == 0 || !impl->namedEntities->exists(name), "entity name must be unique", name);
		entityImpl *e = detail::systemArena().createObject<entityImpl>(impl, name);
		return e;
	}

	entityClass *entityManagerClass::getEntity(uint32 entityName)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return impl->namedEntities->get(entityName, false);
	}

	entityClass *entityManagerClass::getOrNewEntity(uint32 entityName)
	{
		if (hasEntity(entityName))
			return getEntity(entityName);
		return newEntity(entityName);
	}

	bool entityManagerClass::hasEntity(uint32 entityName) const
	{
		return ((entityManagerImpl *)this)->namedEntities->exists(entityName);
	}

	groupClass *entityManagerClass::getAllEntities()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		return &impl->allEntities;
	}

	uint32 entityManagerClass::generateUniqueName()
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		uint32 &index = impl->generateName;
		if (index >= (uint32)-2 || index < (1u << 30))
			index = 1u << 30;
		while (hasEntity(index))
			index = index == (uint32)-2 ? (1u << 30) : index + 1;
		return index++;
	}

	componentClass *entityManagerClass::zPrivateDefineComponent(uintPtr typeSize, void *prototype, bool enumerableEntities)
	{
		entityManagerImpl *impl = (entityManagerImpl *)this;
		componentImpl *c = detail::systemArena().createObject<componentImpl>(impl, typeSize, prototype, enumerableEntities);
		impl->components.push_back(c);
		return c;
	}

	holder<entityManagerClass> newEntityManager(const entityManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<entityManagerClass, entityManagerImpl>(config);
	}

	uintPtr componentClass::getTypeSize() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->typeSize;
	}

	uint32 componentClass::getComponentIndex() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->vectorIndex;
	}

	groupClass *componentClass::getComponentEntities()
	{
		componentImpl *impl = (componentImpl *)this;
		return impl->componentEntities.get();
	}

	entityManagerClass *componentClass::getManager() const
	{
		componentImpl *impl = (componentImpl*)this;
		return impl->manager;
	}

	uint32 entityClass::getName() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->name;
	}

	void entityClass::addGroup(groupClass *group)
	{
		if (hasGroup(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.insert(group);
		((groupImpl*)group)->entities.push_back(impl);
		group->entityAdded.dispatch(this);
	}

	void entityClass::removeGroup(groupClass *group)
	{
		if (!hasGroup(group))
			return;
		entityImpl *impl = (entityImpl*)this;
		impl->groups.erase(group);
		for (std::vector<entityClass*>::reverse_iterator it = ((groupImpl*)group)->entities.rbegin(), et = ((groupImpl*)group)->entities.rend(); it != et; it++)
		{
			if (*it == impl)
			{
				((groupImpl*)group)->entities.erase(--(it.base()));
				break;
			}
		}
		group->entityRemoved.dispatch(this);
	}

	bool entityClass::hasGroup(const groupClass *group) const
	{
		entityImpl *impl = (entityImpl*)this;
		CAGE_ASSERT_RUNTIME(((groupImpl*)group)->manager == impl->manager, "incompatible group");
		return impl->groups.find(const_cast<groupClass*>(group)) != impl->groups.end();
	}

	void entityClass::addComponent(componentClass *component)
	{
		if (hasComponent(component))
			return;
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		if (impl->components.size() < ci->vectorIndex + 1)
			impl->components.resize(ci->vectorIndex + 1);
		void *c = detail::systemArena().allocate(ci->typeSize);
		impl->components[ci->vectorIndex] = c;
		detail::memcpy(c, ci->prototype, ci->typeSize);
		if (ci->componentEntities)
			ci->componentEntities->addEntity(this);
	}

	void entityClass::removeComponent(componentClass *component)
	{
		if (!hasComponent(component))
			return;
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		if (ci->componentEntities)
			ci->componentEntities->removeEntity(this);
		detail::systemArena().deallocate(impl->components[ci->vectorIndex]);
		impl->components[ci->vectorIndex] = nullptr;
	}

	bool entityClass::hasComponent(const componentClass *component) const
	{
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl *)component;
		CAGE_ASSERT_RUNTIME(ci->manager == impl->manager, "incompatible component");
		if (impl->components.size() > ci->vectorIndex)
			return impl->components[ci->vectorIndex] != nullptr;
		return false;
	}

	void entityClass::destroy()
	{
		entityImpl *impl = (entityImpl*)this;
		detail::systemArena().destroy <entityImpl>(impl);
	}

	entityManagerClass *entityClass::getManager() const
	{
		entityImpl *impl = (entityImpl*)this;
		return impl->manager;
	}

	void *entityClass::zPrivateValue(componentClass *component)
	{
		entityImpl *impl = (entityImpl*)this;
		componentImpl *ci = (componentImpl*)component;
		if (impl->components.size() > ci->vectorIndex)
		{
			void *res = impl->components[ci->vectorIndex];
			if (res)
				return res;
		}
		addComponent(component);
		return zPrivateValue(component);
	}

	uint32 groupClass::getGroupIndex() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->vectorIndex;
	}

	uint32 groupClass::entitiesCount() const
	{
		groupImpl *impl = (groupImpl*)this;
		return numeric_cast <uint32> (impl->entities.size());
	}

	entityClass *const *groupClass::entitiesArray()
	{
		groupImpl *impl = (groupImpl*)this;
		if (impl->entities.empty())
			return nullptr;
		return &impl->entities[0];
	}

	pointerRange<entityClass *const> groupClass::entities()
	{
		return { entitiesArray(), entitiesArray() + entitiesCount() };
	}

	void groupClass::entitiesCallback(const delegate<void(entityClass *)> &callback)
	{
		groupImpl *impl = (groupImpl*)this;
		for (auto it = impl->entities.begin(), et = impl->entities.end(); it != et; it++)
			callback(*it);
	}

	void groupClass::addGroup(groupClass *group)
	{
		if (group == this)
			return;
		groupImpl *impl = (groupImpl*)this;
		for (auto it = impl->entities.begin(), et = impl->entities.end(); it != et; it++)
			(*it)->addGroup(group);
	}

	void groupClass::removeGroup(groupClass *group)
	{
		if (group == this)
		{
			clear();
			return;
		}
		groupImpl *impl = (groupImpl*)this;
		for (auto it = impl->entities.begin(), et = impl->entities.end(); it != et; it++)
			(*it)->removeGroup(group);
	}

	void groupClass::clear()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			impl->entities[0]->removeGroup(this);
	}

	void groupClass::destroyAllEntities()
	{
		groupImpl *impl = (groupImpl*)this;
		while (!impl->entities.empty())
			(*impl->entities.rbegin())->destroy();
	}

	entityManagerClass *groupClass::getManager() const
	{
		groupImpl *impl = (groupImpl*)this;
		return impl->manager;
	}
}
