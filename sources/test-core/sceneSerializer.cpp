#include "main.h"

#include <cage-core/math.h>
#include <cage-core/entities.h>

void defineManager(entityManagerClass *man)
{
	man->defineComponent<float>(0, random() < 0.5);
	man->defineComponent<int>(0, random() < 0.5);
	man->defineComponent<vec3>(vec3(), random() < 0.5);
}

void generateEntity(entityClass *e)
{
	bool a = random() < 0.5;
	bool b = random() < 0.5;
	bool c = random() < 0.5 || (!a && !b);
	if (a)
		e->value<float>(e->getManager()->getComponentByIndex(0)) = random().value;
	if (b)
		e->value<int>(e->getManager()->getComponentByIndex(1)) = random(-100, 100);
	if (c)
		e->value<vec3>(e->getManager()->getComponentByIndex(2)) = randomDirection3();
}

void changeEntities(entityManagerClass *man)
{
	for (uint32 round = 0; round < 100; round++)
	{
		uint32 a = random(1, 500);
		if (man->hasEntity(a))
			man->getEntity(a)->destroy();
		else
		{
			entityClass *e = man->newEntity(a);
			generateEntity(e);
		}
	}
	for (uint32 round = 0; round < 5; round++)
	{
		entityClass *e = man->newAnonymousEntity();
		generateEntity(e);
	}
}

void sync(entityManagerClass *a, entityManagerClass *b)
{
	static const uintPtr bufSize = 1024 * 1024 * 32;
	static char buffer[bufSize];
	for (uint32 i = 0; i < 3; i++)
	{
		uintPtr siz = entitiesSerialize(buffer, bufSize, a->getAllEntities(), a->getComponentByIndex(i));
		CAGE_TEST(siz <= bufSize);
		entitiesDeserialize(buffer, siz, b);
	}
}

void check(entityManagerClass *a, entityManagerClass *b)
{
	uint32 cnt = a->getAllEntities()->entitiesCount();
	entityClass *const *ents = a->getAllEntities()->entitiesArray();
	for (uint32 ei = 0; ei < cnt; ei++)
	{
		entityClass *ea = ents[ei];
		uint32 aName = ea->getName();
		if (aName == 0)
			continue;
		CAGE_TEST(b->hasEntity(aName));
		entityClass *eb = b->getEntity(aName);
		for (uint32 i = 0; i < 3; i++)
		{
			if (ea->hasComponent(a->getComponentByIndex(i)))
			{
				CAGE_TEST(eb->hasComponent(b->getComponentByIndex(i)));
				CAGE_TEST(detail::memcmp(ea->unsafeValue(a->getComponentByIndex(i)), eb->unsafeValue(b->getComponentByIndex(i)), a->getComponentByIndex(i)->getTypeSize()) == 0);
			}
		}
	}
}

void testSceneSerialize()
{
	CAGE_TESTCASE("serializer");

	holder<entityManagerClass> manA = newEntityManager(entityManagerCreateConfig());
	defineManager(manA.get());

	holder<entityManagerClass> manB = newEntityManager(entityManagerCreateConfig());
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