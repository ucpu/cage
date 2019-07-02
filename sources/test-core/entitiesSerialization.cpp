#include "main.h"

#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/memoryBuffer.h>

void defineManager(entityManager *man)
{
	man->defineComponent<float>(0, randomChance() < 0.5);
	man->defineComponent<int>(0, randomChance() < 0.5);
	man->defineComponent<vec3>(vec3(), randomChance() < 0.5);
}

void generateEntity(entity *e)
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

void changeEntities(entityManager *man)
{
	for (uint32 round = 0; round < 100; round++)
	{
		uint32 a = randomRange(1, 500);
		if (man->has(a))
			man->get(a)->destroy();
		else
		{
			entity *e = man->create(a);
			generateEntity(e);
		}
	}
	for (uint32 round = 0; round < 5; round++)
	{
		entity *e = man->createAnonymous();
		generateEntity(e);
	}
}

void sync(entityManager *a, entityManager *b)
{
	for (uint32 i = 0; i < 3; i++)
	{
		memoryBuffer buf = entitiesSerialize(a->group(), a->componentByIndex(i));
		entitiesDeserialize(buf, b);
	}
}

void check(entityManager *a, entityManager *b)
{
	uint32 cnt = a->group()->count();
	entity *const *ents = a->group()->array();
	for (uint32 ei = 0; ei < cnt; ei++)
	{
		entity *ea = ents[ei];
		uint32 aName = ea->name();
		if (aName == 0)
			continue;
		CAGE_TEST(b->has(aName));
		entity *eb = b->get(aName);
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

void testSceneSerialize()
{
	CAGE_TESTCASE("serializer");

	holder<entityManager> manA = newEntityManager(entityManagerCreateConfig());
	defineManager(manA.get());

	holder<entityManager> manB = newEntityManager(entityManagerCreateConfig());
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
