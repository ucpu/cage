#include <vector>
#include <map>
#include <set>
#include <algorithm>

#include "main.h"

#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/timer.h>
#include <cage-core/hashString.h>

void testSceneEntities()
{
	CAGE_TESTCASE("entities");

	{
		CAGE_TESTCASE("basic functionality");

		holder<entityManager> manager = newEntityManager(entityManagerCreateConfig());

		entityComponent *position = manager->defineComponent(vec3(), true);
		entityComponent *velocity = manager->defineComponent(vec3(0, -1, 0), false);
		entityComponent *orientation = manager->defineComponent(quat(), true);

		entityGroup *movement = manager->defineGroup();

		entity *terrain = manager->createAnonymous();
		(void)terrain;
		entity *player = manager->createAnonymous();
		player->add(position);
		player->add(velocity);
		player->add(orientation);
		player->add(movement);
		entity *tank = manager->createAnonymous();
		tank->add(position, vec3(100, 0, 50));
		tank->add(orientation);

		CAGE_TEST(player->value<vec3>(velocity) == vec3(0, -1, 0));

		player->value<vec3>(position)[0] = 20;
		player->value<vec3>(position)[1] = 10;
		CAGE_TEST(player->value<vec3>(position) == vec3(20, 10, 0));

		tank->value<vec3>(position)[2] = 100;
		CAGE_TEST(tank->value<vec3>(position) == vec3(100, 0, 100));

		CAGE_TEST(player->has(movement));
		CAGE_TEST(!tank->has(movement));

		CAGE_TEST(player->has(velocity));
		CAGE_TEST(!tank->has(velocity));

		player->remove(position);
		CAGE_TEST(!player->has(position));
	}

	{
		CAGE_TESTCASE("deletions");

		holder<entityManager> manager = newEntityManager(entityManagerCreateConfig());
		for (uint32 i = 0; i < 3; i++)
			manager->defineComponent(vec3(), true);
		for (uint32 i = 0; i < 3; i++)
			manager->defineGroup();

		struct helpStruct
		{
			entityComponent *c;
			eventListener<bool(entity*)> listener;

			helpStruct(entityComponent *c) : c(c)
			{
				listener.bind<helpStruct, &helpStruct::entityDestroyed>(this);
			}

			bool entityDestroyed(entity *e)
			{
				uint32 cnt = c->group()->count();
				if (cnt > 2)
				{
					entity *const *ents = c->group()->array();
					if (*ents != e)
						(*ents)->destroy();
				}
				return false;
			}
		} help(manager->componentByIndex(1));

		manager->componentByIndex(1)->group()->entityRemoved.attach(help.listener);

		for (uint32 cycle = 0; cycle < 30; cycle++)
		{
			for (uint32 i = 0; i < 100; i++)
			{
				entity *e = manager->createUnique();
				for (uint32 j = 0; j < 3; j++)
				{
					if (randomChance() < 0.5)
						e->add(manager->componentByIndex(j));
					if (randomChance() < 0.5)
						e->add(manager->groupByIndex(j));
				}
			}
			for (uint32 j = 0; j < 3; j++)
			{
				if (randomChance() < 0.2)
					manager->groupByIndex(j)->destroy();
			}
		}
	}

	{
		CAGE_TESTCASE("randomized test");

#ifdef CAGE_DEBUG
		static const uint32 totalCycles = 30;
		static const uint32 totalChanges = 50;
		static const uint32 totalComponents = 5;
#else
		static const uint32 totalCycles = 100;
		static const uint32 totalChanges = 500;
		static const uint32 totalComponents = 15;
#endif

		holder<entityManager> manager = newEntityManager(entityManagerCreateConfig());

		for (uint32 i = 0; i < totalComponents; i++)
			manager->defineComponent(vec3(), true);

		typedef std::map<uint32, std::set<uint32>> referenceType;
		referenceType reference;
		uint32 entName = 1;

		for (uint32 cycle = 0; cycle < totalCycles; cycle++)
		{
			// changes
			for (uint32 change = 0; change < totalChanges; change++)
			{
				switch (randomRange(0, 5))
				{
				case 0:
				{ // remove random entity
					if (reference.empty())
						break;
					referenceType::iterator it = reference.begin();
					std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
					manager->get(it->first)->destroy();
					reference.erase(it);
				} break;
				case 1:
				case 2:
				{ // add entity
					CAGE_ASSERT_RUNTIME(reference.find(entName) == reference.end(), entName, reference.size());
					entity *e = manager->create(entName);
					reference[entName];
					for (uint32 i = 0; i < totalComponents; i++)
					{
						if (randomChance() < 0.5)
						{
							e->add(manager->componentByIndex(i));
							reference[entName].insert(i);
						}
					}
					entName++;
				} break;
				case 3:
				{ // add components
					if (reference.empty())
						break;
					referenceType::iterator it = reference.begin();
					std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
					entity *e = manager->get(it->first);
					for (uint32 i = 0; i < totalComponents; i++)
					{
						if (randomChance() < 0.5)
						{
							e->add(manager->componentByIndex(i));
							reference[it->first].insert(i);
						}
					}
				} break;
				case 4:
				{ // remove components
					if (reference.empty())
						break;
					referenceType::iterator it = reference.begin();
					std::advance(it, randomRange((uint32)0, numeric_cast<uint32>(reference.size())));
					entity *e = manager->get(it->first);
					for (uint32 i = 0; i < totalComponents; i++)
					{
						if (randomChance() < 0.5)
						{
							e->remove(manager->componentByIndex(i));
							reference[it->first].erase(i);
						}
					}
				} break;
				}
			}

			// validation
			uint32 entsCnt = manager->group()->count();
			CAGE_TEST(entsCnt == reference.size());
			if (entsCnt == 0)
				continue;
			std::vector<entity*> allEntities;
			allEntities.reserve(entsCnt);
			std::vector<std::vector<entity*> > componentEntities;
			componentEntities.resize(totalComponents);
			for (referenceType::iterator it = reference.begin(), et = reference.end(); it != et; it++)
			{
				entity *e = manager->get(it->first);
				allEntities.push_back(e);
				for (std::set<uint32>::iterator cit = it->second.begin(), cet = it->second.end(); cit != cet; cit++)
					componentEntities[*cit].push_back(e);
			}
			entity *const *entsBufConst = manager->group()->array();
			std::vector<entity*> entsBuf(entsBufConst, entsBufConst + entsCnt);
			std::sort(entsBuf.begin(), entsBuf.end());
			std::sort(allEntities.begin(), allEntities.end());
			CAGE_TEST(detail::memcmp(&entsBuf[0], &allEntities[0], sizeof(entity*) * entsCnt) == 0);
			for (uint32 i = 0; i < totalComponents; i++)
			{
				entsCnt = manager->componentByIndex(i)->group()->count();
				CAGE_TEST(entsCnt == componentEntities[i].size());
				if (entsCnt == 0)
					continue;
				entsBufConst = manager->componentByIndex(i)->group()->array();
				entsBuf = std::vector<entity*>(entsBufConst, entsBufConst + entsCnt);
				std::sort(entsBuf.begin(), entsBuf.end());
				std::sort(componentEntities[i].begin(), componentEntities[i].end());
				CAGE_TEST(detail::memcmp(&entsBuf[0], &componentEntities[i][0], sizeof(entity*) * entsCnt) == 0);
			}
		}
	}

	{
		CAGE_TESTCASE("performance test");

#ifdef CAGE_DEBUG
		static const uint32 totalCycles = 100;
		static const uint32 totalComponents = 10;
		static const uint32 totalGroups = 10;
		static const uint32 usedComponents = 3;
		static const uint32 usedGroups = 3;
		static const uint32 initialEntities = 1000;
#else
		static const uint32 totalCycles = 500;
		static const uint32 totalComponents = 25;
		static const uint32 totalGroups = 25;
		static const uint32 usedComponents = 15;
		static const uint32 usedGroups = 15;
		static const uint32 initialEntities = 5000;
#endif

		holder<entityManager> manager = newEntityManager(entityManagerCreateConfig());
		entityComponent *components[totalComponents];
		entityGroup *groups[totalGroups];

		for (uint32 i = 0; i < totalComponents; i++)
			components[i] = manager->defineComponent(vec3(), true);

		for (uint32 i = 0; i < totalGroups; i++)
			groups[i] = manager->defineGroup();

		std::vector<bool> exists;
		exists.resize(initialEntities + totalCycles * 5);

		uint32 entityNameIndex = 1;
		for (uint32 i = 0; i < initialEntities; i++)
		{
			uint32 n = entityNameIndex++;
			entity *e = manager->create(n);
			for (uint32 j = n % totalComponents, je = min(j + usedComponents, totalComponents); j < je; j++)
				e->add(components[j]);
			for (uint32 j = 0; j < totalGroups; j++)
				if ((n + j) % totalGroups < usedGroups)
					e->add(groups[j]);
			exists[n] = true;
		}

		holder<timer> tmr = newTimer();

		for (uint32 cycle = 0; cycle < totalCycles; cycle++)
		{
			if (randomChance() < 0.2)
			{ // simulate update
				// remove some entities
				for (uint32 i = 0, et = randomRange(1, 20); i < et; i++)
				{
					uint32 n = randomRange(randomRange((uint32)1, entityNameIndex), entityNameIndex);
					if (exists[n])
					{
						exists[n] = false;
						manager->get(n)->destroy();
					}
				}
				// add some new entities
				for (uint32 i = 0, et = randomRange(1, 20); i < et; i++)
				{
					uint32 n = entityNameIndex++;
					entity *e = manager->create(n);
					for (uint32 j = n % totalComponents, je = min(j + usedComponents, totalComponents); j < je; j++)
						e->add(components[j]);
					for (uint32 j = 0; j < totalGroups; j++)
						if ((n + j) % totalGroups < usedGroups)
							e->add(groups[j]);
					exists[n] = true;
				}
			}
			else
			{ // simulate draw
				for (uint32 j = 0; j < totalGroups; j++)
				{
					if (randomChance() < 0.5)
						continue;
					uint32 cnt = groups[j]->count();
					entity *const *ents = groups[j]->array();
					bool w = randomChance() < 0.2;
					for (uint32 k = 0; k < cnt; k++)
					{
						entity *e = ents[k];
						uint32 n = e->name();
						for (uint32 m = n % totalComponents, me = min(m + usedComponents, totalComponents); m < me; m++)
						{
							vec3 &a = e->value <vec3>(components[m]);
							if (w)
								a[0] += a[1] += a[2] += 1;
						}
					}
				}
			}
		}

		CAGE_LOG(severityEnum::Info, "entities performance", string() + "avg time per cycle: " + (tmr->microsSinceStart() / totalCycles) + " us");
	}
}

