#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "main.h"

#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/math.h>
#include <cage-core/timer.h>

namespace
{
	void basicFunctionality()
	{
		CAGE_TESTCASE("basic functionality");

		Holder<EntityManager> manager = newEntityManager();

		EntityComponent *position = manager->defineComponent(Vec3());
		EntityComponent *velocity = manager->defineComponent(Vec3(0, -1, 0));
		EntityComponent *orientation = manager->defineComponent(Quat());

		Entity *terrain = manager->createAnonymous();
		Entity *player = manager->createAnonymous();
		player->add(position);
		player->add(velocity);
		player->add(orientation);
		Entity *tank = manager->createAnonymous();
		tank->add(position, Vec3(100, 0, 50));
		tank->add(orientation);

		CAGE_TEST(player->value<Vec3>(position) == Vec3());
		CAGE_TEST(player->value<Vec3>(velocity) == Vec3(0, -1, 0));

		player->value<Vec3>(position)[0] = 20;
		player->value<Vec3>(position)[1] = 10;
		CAGE_TEST(player->value<Vec3>(position) == Vec3(20, 10, 0));

		tank->value<Vec3>(position)[2] = 100;
		CAGE_TEST(tank->value<Vec3>(position) == Vec3(100, 0, 100));

		CAGE_TEST(player->has(velocity));
		CAGE_TEST(!tank->has(velocity));

		player->remove(position);
		CAGE_TEST(!player->has(position));

		player->remove(velocity);
		CAGE_TEST(!player->has(velocity));

		CAGE_TEST(player->getOrDefault<Vec3>(velocity) == Vec3());
		CAGE_TEST(!player->has(velocity));
	}

	void deletions()
	{
		CAGE_TESTCASE("deletions");

		Holder<EntityManager> manager = newEntityManager();
		for (uint32 i = 0; i < 3; i++)
			manager->defineComponent(Vec3());

		for (uint32 cycle = 0; cycle < 30; cycle++)
		{
			for (uint32 i = 0; i < 100; i++)
			{
				Entity *e = manager->createUnique();
				for (uint32 j = 0; j < 3; j++)
				{
					if (randomChance() < 0.5)
						e->add(manager->componentByDefinition(j));
				}
			}
		}
	}

	void multipleManagers()
	{
		CAGE_TESTCASE("multiple managers");

		Holder<EntityManager> man1 = newEntityManager();
		Holder<EntityManager> man2 = newEntityManager();
		EntityComponent *com1 = man1->defineComponent(Vec3());
		EntityComponent *com2 = man2->defineComponent(Vec3());
		Entity *ent1 = man1->createAnonymous();
		Entity *ent2 = man2->createAnonymous();
		{
			Vec3 &a = ent1->value<Vec3>(com1);
			a = Vec3(1, 2, 3);
		}
		{
			Vec3 &b = ent2->value<Vec3>(com2);
			b = Vec3(4, 5, 6);
		}
		{
			CAGE_TEST_ASSERTED(ent1->value<Vec3>(com2));
			CAGE_TEST_ASSERTED(ent1->value<Vec2>(com1));
		}
	}

	void componentsWithAlignment()
	{
		CAGE_TESTCASE("components with alignment");

		Holder<EntityManager> manager = newEntityManager();

		struct alignas(32) S
		{
			Vec3 data;
		};

		EntityComponent *c = manager->defineComponent(S());

		for (uint32 i = 0; i < 10000; i++)
		{
			Entity *e = manager->createAnonymous();
			S &s = e->value<S>(c);
			CAGE_TEST(((uintPtr)&s % alignof(S)) == 0);
		}
	}

	void multipleComponentsOfSameType()
	{
		Holder<EntityManager> man = newEntityManager();

		EntityComponent *ca = man->defineComponent(Vec3(13));
		EntityComponent *cb = man->defineComponent(Vec3(42));
		CAGE_TEST(ca != cb);
		CAGE_TEST(ca->typeIndex() == cb->typeIndex());
		CAGE_TEST(ca->definitionIndex() == 0);
		CAGE_TEST(cb->definitionIndex() == 1);

		Entity *e = man->createAnonymous();
		e->add(ca, Vec3(1));
		e->add(cb, Vec3(2));

		CAGE_TEST(e->value<Vec3>() == Vec3(1));
		CAGE_TEST(e->value<Vec3>(ca) == Vec3(1));
		CAGE_TEST(e->value<Vec3>(cb) == Vec3(2));

		CAGE_TEST(e->has(ca));
		CAGE_TEST(e->has(cb));
		CAGE_TEST(e->has<Vec3>());

		e->remove<Vec3>();

		CAGE_TEST(!e->has(ca));
		CAGE_TEST(e->has(cb));
		CAGE_TEST(!e->has<Vec3>()); // component by type still refers to ca, not cb

		CAGE_TEST(e->value<Vec3>() == Vec3(13)); // adding component ca again has default value
	}

	void callbacks()
	{
		CAGE_TESTCASE("callbacks");

		struct Callbacks
		{
			uint32 added = 0;
			uint32 removed = 0;

			EventListener<bool(Entity *)> addListener;
			EventListener<bool(Entity *)> removeListener;

			Callbacks()
			{
				addListener.bind([&](Entity *) { added++; });
				removeListener.bind([&](Entity *) { removed++; });
			}
		} manCbs;

		Holder<EntityManager> man = newEntityManager();
		manCbs.addListener.attach(man->entityAdded);
		manCbs.removeListener.attach(man->entityRemoved);
		EntityComponent *pos = man->defineComponent(Vec3());
		EntityComponent *ori = man->defineComponent(Quat());

		for (uint32 i = 0; i < 100; i++)
		{
			Entity *e = man->createUnique();
			if ((i % 3) == 0)
				e->add(pos);
			if ((i % 5) == 0)
				e->add(ori);
		}

		CAGE_TEST(manCbs.added == 100);
		CAGE_TEST(manCbs.removed == 0);

		ori->destroy();

		CAGE_TEST(manCbs.added == 100);
		CAGE_TEST(manCbs.removed == 20);
	}

	void randomizedTests()
	{
		CAGE_TESTCASE("randomized test");

#ifdef CAGE_DEBUG
		constexpr uint32 TotalCycles = 30;
		constexpr uint32 TotalChanges = 50;
		constexpr uint32 TotalComponents = 5;
#else
		constexpr uint32 TotalCycles = 100;
		constexpr uint32 TotalChanges = 500;
		constexpr uint32 TotalComponents = 15;
#endif

		Holder<EntityManager> manager = newEntityManager();

		for (uint32 i = 0; i < TotalComponents; i++)
			manager->defineComponent(Vec3());

		typedef std::map<uint32, std::set<uint32>> Reference;
		Reference reference;
		uint32 entName = 1;

		for (uint32 cycle = 0; cycle < TotalCycles; cycle++)
		{
			// changes
			for (uint32 change = 0; change < TotalChanges; change++)
			{
				switch (randomRange(0, 6))
				{
					case 0:
					{ // remove random entity
						if (reference.empty())
							break;
						auto it = reference.begin();
						std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
						manager->get(it->first)->destroy();
						reference.erase(it);
					}
					break;
					case 1:
					case 2:
					{ // add entity
						CAGE_ASSERT(reference.find(entName) == reference.end());
						Entity *e = manager->create(entName);
						reference[entName];
						for (uint32 i = 0; i < TotalComponents; i++)
						{
							if (randomChance() < 0.5)
							{
								e->add(manager->componentByDefinition(i));
								reference[entName].insert(i);
							}
						}
						entName++;
					}
					break;
					case 3:
					{ // add components
						if (reference.empty())
							break;
						auto it = reference.begin();
						std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
						Entity *e = manager->get(it->first);
						for (uint32 i = 0; i < TotalComponents; i++)
						{
							if (randomChance() < 0.5)
							{
								e->add(manager->componentByDefinition(i));
								reference[it->first].insert(i);
							}
						}
					}
					break;
					case 4:
					{ // remove components
						if (reference.empty())
							break;
						auto it = reference.begin();
						std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
						Entity *e = manager->get(it->first);
						for (uint32 i = 0; i < TotalComponents; i++)
						{
							if (randomChance() < 0.5)
							{
								e->remove(manager->componentByDefinition(i));
								reference[it->first].erase(i);
							}
						}
					}
					break;
					case 5:
					{ // test components
						if (reference.empty())
							break;
						auto it = reference.begin();
						std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
						Entity *e = manager->get(it->first);
						for (uint32 i = 0; i < TotalComponents; i++)
							CAGE_TEST(!!reference[it->first].count(i) == e->has(manager->componentByDefinition(i)));
					}
					break;
				}
			}

			// validation
			CAGE_TEST(manager->count() == reference.size());
			if (reference.empty())
				continue;
			std::vector<Entity *> allEntities;
			allEntities.reserve(reference.size());
			std::vector<std::vector<Entity *>> componentEntities;
			componentEntities.resize(TotalComponents);
			for (const auto &it : reference)
			{
				Entity *e = manager->get(it.first);
				allEntities.push_back(e);
				for (const auto &cit : it.second)
					componentEntities[cit].push_back(e);
			}
			std::vector<Entity *> entsBuf(manager->entities().begin(), manager->entities().end());
			std::sort(entsBuf.begin(), entsBuf.end());
			std::sort(allEntities.begin(), allEntities.end());
			CAGE_TEST(detail::memcmp(entsBuf.data(), allEntities.data(), sizeof(Entity *) * reference.size()) == 0);
			for (uint32 i = 0; i < TotalComponents; i++)
			{
				const EntityComponent *grp = manager->componentByDefinition(i);
				CAGE_TEST(grp->count() == componentEntities[i].size());
				if (componentEntities[i].empty())
					continue;
				entsBuf = std::vector<Entity *>(grp->entities().begin(), grp->entities().end());
				std::sort(entsBuf.begin(), entsBuf.end());
				std::sort(componentEntities[i].begin(), componentEntities[i].end());
				CAGE_TEST(detail::memcmp(entsBuf.data(), componentEntities[i].data(), sizeof(Entity *) * componentEntities[i].size()) == 0);
			}
		}
	}

	void performanceTypeVsComponent()
	{
		CAGE_TESTCASE("performance type vs component");

		Holder<EntityManager> man = newEntityManager();

		EntityComponent *comps[3] = {};
		comps[0] = man->defineComponent(Vec3());
		comps[1] = man->defineComponent(uint32());
		comps[2] = man->defineComponent(Real());

		for (uint32 i = 0; i < 10000; i++)
		{
			Entity *e = man->createUnique();
			if ((i % 2) == 1)
				e->value<Vec3>();
			if ((i % 3) > 0)
				e->value<uint32>();
			if ((i % 7) < 4)
				e->value<Real>();
		}

		for (uint32 round = 0; round < 3; round++)
		{
			Holder<Timer> tmr = newTimer();

			for (Entity *e : comps[0]->entities())
				e->value<Vec3>() += 1;
			for (Entity *e : comps[1]->entities())
				e->value<uint32>() += 1;
			for (Entity *e : comps[2]->entities())
				e->value<Real>() += 1;

			CAGE_LOG(SeverityEnum::Info, "entities performance", Stringizer() + "time for access with type: " + (tmr->duration()) + " us");
		}

		for (uint32 round = 0; round < 3; round++)
		{
			Holder<Timer> tmr = newTimer();

			for (Entity *e : comps[0]->entities())
				e->value<Vec3>(comps[0]) += 1;
			for (Entity *e : comps[1]->entities())
				e->value<uint32>(comps[1]) += 1;
			for (Entity *e : comps[2]->entities())
				e->value<Real>(comps[2]) += 1;

			CAGE_LOG(SeverityEnum::Info, "entities performance", Stringizer() + "time for access with component: " + (tmr->duration()) + " us");
		}
	}

	void performanceSimulationTest()
	{
		CAGE_TESTCASE("performance simulation test");

#ifdef CAGE_DEBUG
		constexpr uint32 TotalCycles = 100;
		constexpr uint32 TotalComponents = 10;
		constexpr uint32 UsedComponents = 3;
		constexpr uint32 UsedGroups = 3;
		constexpr uint32 InitialEntities = 1000;
#else
		constexpr uint32 TotalCycles = 500;
		constexpr uint32 TotalComponents = 25;
		constexpr uint32 UsedComponents = 15;
		constexpr uint32 UsedGroups = 15;
		constexpr uint32 InitialEntities = 5000;
#endif

		Holder<EntityManager> manager = newEntityManager();
		EntityComponent *components[TotalComponents];

		for (uint32 i = 0; i < TotalComponents; i++)
			components[i] = manager->defineComponent(Vec3());

		std::vector<bool> exists;
		exists.resize(InitialEntities + TotalCycles * 5);

		uint32 entityNameIndex = 1;
		for (uint32 i = 0; i < InitialEntities; i++)
		{
			const uint32 n = entityNameIndex++;
			Entity *e = manager->create(n);
			for (uint32 j = n % TotalComponents, je = min(j + UsedComponents, TotalComponents); j < je; j++)
				e->add(components[j]);
			exists[n] = true;
		}

		Holder<Timer> tmr = newTimer();

		for (uint32 cycle = 0; cycle < TotalCycles; cycle++)
		{
			if (randomChance() < 0.2)
			{ // simulate update
				// remove some entities
				for (uint32 i = 0, et = randomRange(1, 20); i < et; i++)
				{
					const uint32 n = randomRange(randomRange((uint32)1, entityNameIndex), entityNameIndex);
					if (exists[n])
					{
						exists[n] = false;
						manager->get(n)->destroy();
					}
				}
				// add some new entities
				for (uint32 i = 0, et = randomRange(1, 20); i < et; i++)
				{
					const uint32 n = entityNameIndex++;
					Entity *e = manager->create(n);
					const uint32 je = min((n % TotalComponents) + UsedComponents, TotalComponents);
					for (uint32 j = n % TotalComponents; j < je; j++)
						e->add(components[j]);
					exists[n] = true;
				}
			}
			else
			{ // simulate draw
				for (uint32 i = 0; i < TotalComponents; i++)
				{
					if (randomChance() < 0.5)
						continue;
					const bool w = randomChance() < 0.2;
					for (Entity *e : components[i]->entities())
					{
						const uint32 n = e->id();
						const uint32 je = min((n % TotalComponents) + UsedComponents, TotalComponents);
						for (uint32 j = n % TotalComponents; j < je; j++)
						{
							Vec3 &a = e->value<Vec3>(components[j]);
							if (w)
								a[0] += a[1] += a[2] += 1;
						}
					}
				}
			}
		}

		CAGE_LOG(SeverityEnum::Info, "entities performance", Stringizer() + "simulation avg time per cycle: " + (tmr->duration() / TotalCycles) + " us");
	}
}

void testEntities()
{
	CAGE_TESTCASE("entities");

	basicFunctionality();
	deletions();
	multipleManagers();
	componentsWithAlignment();
	multipleComponentsOfSameType();
	callbacks();
	randomizedTests();
	performanceTypeVsComponent();
	performanceSimulationTest();
}
