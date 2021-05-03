#include "main.h"

#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>

namespace
{
	void defineManager(EntityManager *man)
	{
		man->defineComponent<float>(0);
		man->defineComponent<int>(0);
		man->defineComponent(vec3());
	}

	void generateEntity(Entity *e)
	{
		const bool a = randomChance() < 0.5;
		const bool b = randomChance() < 0.5;
		const bool c = randomChance() < 0.5 || (!a && !b);
		if (a)
			e->value<float>(e->manager()->componentByOrder(0)) = randomChance().value;
		if (b)
			e->value<int>(e->manager()->componentByOrder(1)) = randomRange(-100, 100);
		if (c)
			e->value<vec3>(e->manager()->componentByOrder(2)) = randomDirection3();
	}

	void changeEntities(EntityManager *man)
	{
		for (uint32 round = 0; round < 100; round++)
		{
			uint32 a = randomRange(1, 500);
			if (man->has(a))
				man->get(a)->destroy();
			else
			{
				Entity *e = man->create(a);
				generateEntity(e);
			}
		}
		for (uint32 round = 0; round < 5; round++)
		{
			Entity *e = man->createAnonymous();
			generateEntity(e);
		}
	}

	void sync(EntityManager *a, EntityManager *b)
	{
		for (uint32 i = 0; i < 3; i++)
		{
			Holder<PointerRange<char>> buf = entitiesSerialize(a->group(), a->componentByOrder(i));
			entitiesDeserialize(buf, b);
		}
	}

	void check(EntityManager *a, EntityManager *b)
	{
		for (Entity *ea : a->entities())
		{
			uint32 aName = ea->name();
			if (aName == 0)
				continue;
			CAGE_TEST(b->has(aName));
			Entity *eb = b->get(aName);
			for (uint32 i = 0; i < 3; i++)
			{
				if (ea->has(a->componentByOrder(i)))
				{
					CAGE_TEST(eb->has(b->componentByOrder(i)));
					CAGE_TEST(detail::memcmp(ea->unsafeValue(a->componentByOrder(i)), eb->unsafeValue(b->componentByOrder(i)), detail::typeSize(a->componentByOrder(i)->typeIndex())) == 0);
				}
			}
		}
	}
}

void testEntitiesSerialization()
{
	CAGE_TESTCASE("entities serialization");

	Holder<EntityManager> manA = newEntityManager();
	defineManager(+manA);

	Holder<EntityManager> manB = newEntityManager();
	defineManager(+manB);

	CAGE_TESTCASE("randomized test");
	for (uint32 round = 0; round < 10; round++)
	{
		changeEntities(+manA);
		changeEntities(+manB);
		sync(+manA, +manB);
		check(+manA, +manB);
	}
}
