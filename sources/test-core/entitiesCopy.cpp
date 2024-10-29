#include <map>

#include "main.h"

#include <cage-core/entities.h>
#include <cage-core/entitiesCopy.h>
#include <cage-core/math.h>

namespace
{
	void generateEntity(Entity *e)
	{
		for (EntityComponent *c : e->manager()->components())
		{
			if (randomChance() < 0.5)
				continue;
			if (c->typeIndex() == detail::typeIndex<float>())
				e->value<float>(c) = randomChance().value;
			else if (c->typeIndex() == detail::typeIndex<int>())
				e->value<int>(c) = randomRange(-100, 100);
			else if (c->typeIndex() == detail::typeIndex<Vec3>())
				e->value<Vec3>(c) = randomDirection3();
		}
	}

	void changeEntities(EntityManager *man)
	{
		for (uint32 round = 0; round < 100; round++)
		{
			uint32 a = randomRange(1, 500);
			if (man->exists(a))
			{
				Entity *e = man->get(a);
				if (randomChance() < 0.5)
					e->destroy();
				else
				{
					for (EntityComponent *c : man->components())
						e->remove(c);
					generateEntity(e);
				}
			}
			else
				generateEntity(man->create(a));
		}
		for (uint32 round = 0; round < 5; round++)
		{
			Entity *e = man->createAnonymous();
			generateEntity(e);
		}
	}

	uint32 indexOfComponentByType(EntityManager *m, EntityComponent *c)
	{
		uint32 i = 0;
		for (EntityComponent *d : m->componentsByType(c->typeIndex()))
		{
			if (d == c)
				return i;
			i++;
		}
		CAGE_TEST(false);
	}

	EntityComponent *componentByTypeAtIndex(EntityManager *m, uint32 typeIndex, uint32 subtypeDefineIndex)
	{
		return m->componentsByType(typeIndex)[subtypeDefineIndex];
	}

	void check(EntityManager *a, EntityManager *b)
	{
		CAGE_TEST(a->count() > 0);
		CAGE_TEST(a->count() == b->count());
		CAGE_TEST(a->componentsCount() == b->componentsCount());

		// verify that names are correctly paired
		for (Entity *ae : a->entities())
		{
			if (ae->id())
				CAGE_TEST(b->exists(ae->id()));
		}
		for (Entity *be : b->entities())
		{
			if (be->id())
				CAGE_TEST(a->exists(be->id()));
		}

		// generate components mapping
		std::map<EntityComponent *, EntityComponent *> mapping;
		for (EntityComponent *ac : a->components())
			mapping[ac] = componentByTypeAtIndex(b, ac->typeIndex(), indexOfComponentByType(a, ac));
		CAGE_TEST(mapping.size() == a->componentsCount());
		for (const auto &it : mapping)
			CAGE_TEST(it.first->typeIndex() == it.second->typeIndex());

		// verify values
		for (Entity *ae : a->entities())
		{
			if (!ae->id())
				continue;
			Entity *be = b->get(ae->id());

			// compare values of all types in generic way
			for (const auto &it : mapping)
			{
				CAGE_TEST(ae->has(it.first) == be->has(it.second));
				if (ae->has(it.first))
					CAGE_TEST(detail::memcmp(ae->unsafeValue(it.first), be->unsafeValue(it.second), detail::typeSizeByIndex(it.first->typeIndex())) == 0);
			}

			// compare values of specific types
			if (ae->has<float>())
				CAGE_TEST(ae->value<float>() == be->value<float>());
			if (ae->has<int>())
				CAGE_TEST(ae->value<int>() == be->value<int>());
			if (ae->has<Vec3>())
				CAGE_TEST(ae->value<Vec3>() == be->value<Vec3>());
		}
	}
}

void testEntitiesCopy()
{
	CAGE_TESTCASE("entities copy");

	Holder<EntityManager> am = newEntityManager();
	Holder<EntityManager> bm = newEntityManager();

	am->defineComponent(float());
	am->defineComponent(int());
	am->defineComponent(Vec3());
	am->defineComponent(Vec3());
	am->defineComponent(float());
	am->defineComponent(int());
	am->defineComponent(Vec3());

	for (uint32 round = 0; round < 20; round++)
	{
		changeEntities(+am);
		entitiesCopy({ +am, +bm });
		check(+am, +bm);
	}
}
