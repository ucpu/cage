#include "main.h"

#include <cage-core/entitiesVisitor.h>
#include <cage-core/math.h>
#include <cage-core/timer.h>

namespace
{
	void visitorBasics()
	{
		CAGE_TESTCASE("visitor basics");

		Holder<EntityManager> man = newEntityManager();

		man->defineComponent(vec3());
		man->defineComponent(real());
		man->defineComponent(uint32());

		Entity *a = man->createUnique();
		Entity *b = man->createUnique();
		Entity *c = man->createUnique();

		a->value<vec3>() = vec3(3);
		a->value<real>() = 3;
		a->value<uint32>() = 3;

		b->value<vec3>() = vec3(4);
		b->value<real>() = 4;

		c->value<vec3>() = vec3(5);
		c->value<uint32>() = 5;

		entitiesVisitor(+man, [](vec3 &v, const real &r) { v[0] *= r; });
		entitiesVisitor(+man, [](vec3 &v, const uint32 &u) { v[1] *= u; });
		entitiesVisitor(+man, [](vec3 &v, const real &r, const uint32 &u) { v[2] = 0; });
		entitiesVisitor(+man, [](uint32 &u) { u = 0; });

		CAGE_TEST(a->value<vec3>() == vec3(9, 9, 0));
		CAGE_TEST(b->value<vec3>() == vec3(16, 4, 4));
		CAGE_TEST(c->value<vec3>() == vec3(5, 25, 5));
		CAGE_TEST(a->value<uint32>() == 0);

		CAGE_TEST_THROWN(entitiesVisitor(+man, [](quat &) {}));
		CAGE_TEST_THROWN(entitiesVisitor(+man, [](Entity *, quat &, real &) {}));
	}

	void visitorWithEntity()
	{
		CAGE_TESTCASE("visitor with entity");

		Holder<EntityManager> man = newEntityManager();

		man->defineComponent(real());
		man->defineComponent(uint32());

		Entity *a = man->createUnique();
		Entity *b = man->createUnique();
		Entity *c = man->createUnique();

		a->value<real>() = 3;
		a->value<uint32>() = 3;
		b->value<real>() = 4;
		c->value<uint32>() = 5;

		entitiesVisitor(+man, [](Entity *e, uint32 &u) { u = e->name(); });

		CAGE_TEST(a->value<uint32>() == a->name());
		CAGE_TEST(c->value<uint32>() == c->name());

		entitiesVisitor(+man, [](Entity *e, uint32 &u, real &r) { r = e->name() - u; });

		uint32 cnt = 0;
		entitiesVisitor(+man, [&](Entity *) { cnt++; });
		CAGE_TEST(cnt == man->count());
	}

	void performanceTest()
	{
		CAGE_TESTCASE("performance");

#ifdef CAGE_DEBUG
		constexpr uint32 TotalCycles = 5;
		constexpr uint32 TotalEntities = 5000;
#else
		constexpr uint32 TotalCycles = 50;
		constexpr uint32 TotalEntities = 50000;
#endif

		Holder<EntityManager> man = newEntityManager();

		man->defineComponent(vec3());
		man->defineComponent(real());
		man->defineComponent(uint32());

		for (uint32 i = 0; i < TotalEntities; i++)
		{
			Entity *e = man->createUnique();
			if ((i % 2) == 1)
				e->value<vec3>();
			if ((i % 3) > 0)
				e->value<real>();
			if ((i % 7) < 4)
				e->value<uint32>();
		}

		Holder<Timer> tmr = newTimer();

		for (uint32 cycle = 0; cycle < TotalCycles; cycle++)
		{
			entitiesVisitor(+man, [](vec3 &v, const real &r) { v[1] += r; });
			entitiesVisitor(+man, [](vec3 &v, const real &r, uint32 &u) { v[0] += r; u++; });
			entitiesVisitor(+man, [](uint32 &u) { u++; });
		}

		CAGE_LOG(SeverityEnum::Info, "visitor performance", stringizer() + "visitor avg time per cycle: " + (tmr->duration() / TotalCycles) + " us");
	}
}

void testEntitiesVisitor()
{
	CAGE_TESTCASE("entities visitor");

	visitorBasics();
	visitorWithEntity();
	performanceTest();
}

