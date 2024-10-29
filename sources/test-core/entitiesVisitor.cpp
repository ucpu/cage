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

		man->defineComponent(Vec3());
		man->defineComponent(Real());
		man->defineComponent(uint32());

		Entity *a = man->createUnique();
		Entity *b = man->createUnique();
		Entity *c = man->createUnique();

		a->value<Vec3>() = Vec3(3);
		a->value<Real>() = 3;
		a->value<uint32>() = 3;

		b->value<Vec3>() = Vec3(4);
		b->value<Real>() = 4;

		c->value<Vec3>() = Vec3(5);
		c->value<uint32>() = 5;

		entitiesVisitor([](Vec3 &v, const Real &r) { v[0] *= r; }, +man, false);
		entitiesVisitor([](Vec3 &v, const uint32 &u) { v[1] *= u; }, +man, false);
		entitiesVisitor([](Vec3 &v, const Real &r, const uint32 &u) { v[2] = 0; }, +man, false);
		entitiesVisitor([](uint32 &u) { u = 0; }, +man, false);

		CAGE_TEST(a->value<Vec3>() == Vec3(9, 9, 0));
		CAGE_TEST(b->value<Vec3>() == Vec3(16, 4, 4));
		CAGE_TEST(c->value<Vec3>() == Vec3(5, 25, 5));
		CAGE_TEST(a->value<uint32>() == 0);

		CAGE_TEST_THROWN(entitiesVisitor([](Quat &) {}, +man, false));
		CAGE_TEST_THROWN(entitiesVisitor([](Entity *, Quat &, Real &) {}, +man, false));
	}

	void visitorWithEntity()
	{
		CAGE_TESTCASE("visitor with entity");

		Holder<EntityManager> man = newEntityManager();

		man->defineComponent(Real());
		man->defineComponent(uint32());

		Entity *a = man->createUnique();
		Entity *b = man->createUnique();
		Entity *c = man->createUnique();

		a->value<Real>() = 3;
		a->value<uint32>() = 3;
		b->value<Real>() = 4;
		c->value<uint32>() = 5;

		entitiesVisitor([](Entity *e, uint32 &u) { u = e->id(); }, +man, false);

		CAGE_TEST(a->value<uint32>() == a->id());
		CAGE_TEST(c->value<uint32>() == c->id());

		entitiesVisitor([](Entity *e, uint32 &u, Real &r) { r = e->id() - u; }, +man, false);

		uint32 cnt = 0;
		entitiesVisitor([&](Entity *) { cnt++; }, +man, false);
		CAGE_TEST(cnt == man->count());
	}

	void visitorWithModifications()
	{
		CAGE_TESTCASE("visitor with modifications");

		Holder<EntityManager> man = newEntityManager();

		man->defineComponent(Real());
		man->defineComponent(uint32());

		Entity *a = man->createUnique();
		Entity *b = man->createUnique();
		Entity *c = man->createUnique();

		a->value<Real>() = 3;
		a->value<uint32>() = 3;
		b->value<Real>() = 4;
		c->value<uint32>() = 5;

		CAGE_TEST(man->component<Real>()->count() == 2);
		CAGE_TEST(man->component<uint32>()->count() == 2);
		CAGE_TEST(man->count() == 3);

		uint32 cnt = 0;
		entitiesVisitor(
			[&](Entity *e, const uint32 &v)
			{
				cnt++;
				switch (v)
				{
					case 3:
						man->createUnique()->value<Real>();
						break;
					case 5:
						e->destroy();
						break;
				}
			},
			+man, true);

		CAGE_TEST(cnt == 2);
		CAGE_TEST(man->component<Real>()->count() == 3);
		CAGE_TEST(man->component<uint32>()->count() == 1);
		CAGE_TEST(man->count() == 3);
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

		man->defineComponent(Vec3());
		man->defineComponent(Real());
		man->defineComponent(uint32());

		for (uint32 i = 0; i < TotalEntities; i++)
		{
			Entity *e = man->createUnique();
			if ((i % 2) == 1)
				e->value<Vec3>();
			if ((i % 3) > 0)
				e->value<Real>();
			if ((i % 7) < 4)
				e->value<uint32>();
		}

		Holder<Timer> tmr = newTimer();

		for (uint32 cycle = 0; cycle < TotalCycles; cycle++)
		{
			entitiesVisitor([](Vec3 &v, const Real &r) { v[1] += r; }, +man, false);
			entitiesVisitor(
				[](Vec3 &v, const Real &r, uint32 &u)
				{
					v[0] += r;
					u++;
				},
				+man, false);
			entitiesVisitor([](uint32 &u) { u++; }, +man, false);
		}

		CAGE_LOG(SeverityEnum::Info, "visitor performance", Stringizer() + "visitor avg time per cycle: " + (tmr->duration() / TotalCycles) + " us");
	}
}

void testEntitiesVisitor()
{
	CAGE_TESTCASE("entities visitor");

	visitorBasics();
	visitorWithEntity();
	visitorWithModifications();
	performanceTest();
}
