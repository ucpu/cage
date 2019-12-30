#include "main.h"

#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>

namespace
{
	void defineManager(EntityManager *man)
	{
		man->defineComponent<float>(0, randomChance() < 0.5);
		man->defineComponent<int>(0, randomChance() < 0.5);
		man->defineComponent<vec3>(vec3(), randomChance() < 0.5);
	}

	void generateEntity(Entity *e)
	{
		bool a = randomChance() < 0.5;
		bool b = randomChance() < 0.5;
		bool c = randomChance() < 0.5 || (!a && !b);
		if (a)
			e->value<float>(e->manager()->componentByIndex(0)) = randomChance().value;
		if (b)
			e->value<int>(e->manager()->componentByIndex(1)) = randomRange(-100, 100);
		if (c)
			e->value<vec3>(e->manager()->componentByIndex(2)) = randomDirection3();
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
			MemoryBuffer buf = entitiesSerialize(a->group(), a->componentByIndex(i));
			entitiesDeserialize(buf, b);
		}
	}

	void check(EntityManager *a, EntityManager *b)
	{
		uint32 cnt = a->group()->count();
		Entity *const *ents = a->group()->array();
		for (uint32 ei = 0; ei < cnt; ei++)
		{
			Entity *ea = ents[ei];
			uint32 aName = ea->name();
			if (aName == 0)
				continue;
			CAGE_TEST(b->has(aName));
			Entity *eb = b->get(aName);
			for (uint32 i = 0; i < 3; i++)
			{
				if (ea->has(a->componentByIndex(i)))
				{
					CAGE_TEST(eb->has(b->componentByIndex(i)));
					CAGE_TEST(detail::memcmp(ea->unsafeValue(a->componentByIndex(i)), eb->unsafeValue(b->componentByIndex(i)), a->componentByIndex(i)->typeSize()) == 0);
				}
			}
		}
	}
}

void testEntitiesSerialization()
{
	CAGE_TESTCASE("entities serialization");

	Holder<EntityManager> manA = newEntityManager(EntityManagerCreateConfig());
	defineManager(manA.get());

	Holder<EntityManager> manB = newEntityManager(EntityManagerCreateConfig());
	defineManager(manB.get());

	CAGE_TESTCASE("randomized test");
	for (uint32 round = 0; round < 10; round++)
	{
		changeEntities(manA.get());
		changeEntities(manB.get());
		sync(manA.get(), manB.get());
		check(manA.get(), manB.get());
	}
}
