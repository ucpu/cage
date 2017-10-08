#include <vector>
#include <map>
#include <set>
#include <algorithm>

#include "main.h"

#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/utility/timer.h>
#include <cage-core/utility/hashString.h>

void testSceneEntities()
{
	CAGE_TESTCASE("entities");

	{
		CAGE_TESTCASE("basic functionality");

		holder<entityManagerClass> manager = newEntityManager(entityManagerCreateConfig());

		componentClass *position = manager->defineComponent(vec3(), true);
		componentClass *velocity = manager->defineComponent(vec3(0, -1, 0), false);
		componentClass *orientation = manager->defineComponent(quat(), true);

		groupClass *movement = manager->defineGroup();

		entityClass *terrain = manager->newEntity();
		entityClass *player = manager->newEntity();
		player->addComponent(position);
		player->addComponent(velocity);
		player->addComponent(orientation);
		player->addGroup(movement);
		entityClass *tank = manager->newEntity();
		tank->addComponent(position, vec3(100, 0, 50));
		tank->addComponent(orientation);

		CAGE_TEST(player->value<vec3>(velocity) == vec3(0, -1, 0));

		player->value<vec3>(position)[0] = 20;
		player->value<vec3>(position)[1] = 10;
		CAGE_TEST(player->value<vec3>(position) == vec3(20, 10, 0));

		tank->value<vec3>(position)[2] = 100;
		CAGE_TEST(tank->value<vec3>(position) == vec3(100, 0, 100));

		CAGE_TEST(player->hasGroup(movement));
		CAGE_TEST(!tank->hasGroup(movement));

		CAGE_TEST(player->hasComponent(velocity));
		CAGE_TEST(!tank->hasComponent(velocity));

		player->removeComponent(position);
		CAGE_TEST(!player->hasComponent(position));
	}

	{
		CAGE_TESTCASE("deletions");

		holder<entityManagerClass> manager = newEntityManager(entityManagerCreateConfig());
		for (uint32 i = 0; i < 3; i++)
			manager->defineComponent(vec3(), true);
		for (uint32 i = 0; i < 3; i++)
			manager->defineGroup();

		struct helpStruct
		{
			componentClass *c;
			eventListener<bool(entityClass*)> listener;

			helpStruct(componentClass *c) : c(c)
			{
				listener.bind<helpStruct, &helpStruct::entityDestroyed>(this);
			}

			bool entityDestroyed(entityClass *e)
			{
				uint32 cnt = c->getComponentEntities()->entitiesCount();
				if (cnt > 2)
				{
					entityClass *const *ents = c->getComponentEntities()->entitiesArray();
					if (*ents != e)
						(*ents)->destroy();
				}
				return false;
			}
		} help(manager->getComponentByIndex(1));

		manager->getComponentByIndex(1)->getComponentEntities()->entityRemoved.add(help.listener);

		for (uint32 cycle = 0; cycle < 30; cycle++)
		{
			for (uint32 i = 0; i < 100; i++)
			{
				entityClass *e = manager->newEntity(manager->generateUniqueName());
				for (uint32 j = 0; j < 3; j++)
				{
					if (cage::random() < 0.5)
						e->addComponent(manager->getComponentByIndex(j));
					if (cage::random() < 0.5)
						e->addGroup(manager->getGroupByIndex(j));
				}
			}
			for (uint32 j = 0; j < 3; j++)
			{
				if (cage::random() < 0.2)
					manager->getGroupByIndex(j)->destroyAllEntities();
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

		holder<entityManagerClass> manager = newEntityManager(entityManagerCreateConfig());

		for (uint32 i = 0; i < totalComponents; i++)
			manager->defineComponent(vec3(), true);

		typedef std::map<uint32, std::set<uint32> > referenceType;
		referenceType reference;
		uint32 entName = 1;

		for (uint32 cycle = 0; cycle < totalCycles; cycle++)
		{
			// changes
			for (uint32 change = 0; change < totalChanges; change++)
			{
				switch (random(0, 5))
				{
				case 0:
				{ // remove random entity
					if (reference.empty())
						break;
					referenceType::iterator it = reference.begin();
					std::advance(it, random(0, numeric_cast<uint32>(reference.size())));
					manager->getEntity(it->first)->destroy();
					reference.erase(it);
				} break;
				case 1:
				case 2:
				{ // add entity
					CAGE_ASSERT_RUNTIME(reference.find(entName) == reference.end(), entName, reference.size());
					entityClass *e = manager->newEntity(entName);
					reference[entName];
					for (uint32 i = 0; i < totalComponents; i++)
					{
						if (cage::random() < 0.5)
						{
							e->addComponent(manager->getComponentByIndex(i));
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
					std::advance(it, random(0, numeric_cast<uint32>(reference.size())));
					entityClass *e = manager->getEntity(it->first);
					for (uint32 i = 0; i < totalComponents; i++)
					{
						if (cage::random() < 0.5)
						{
							e->addComponent(manager->getComponentByIndex(i));
							reference[it->first].insert(i);
						}
					}
				} break;
				case 4:
				{ // remove components
					if (reference.empty())
						break;
					referenceType::iterator it = reference.begin();
					std::advance(it, random(0, numeric_cast<uint32>(reference.size())));
					entityClass *e = manager->getEntity(it->first);
					for (uint32 i = 0; i < totalComponents; i++)
					{
						if (cage::random() < 0.5)
						{
							e->removeComponent(manager->getComponentByIndex(i));
							reference[it->first].erase(i);
						}
					}
				} break;
				}
			}

			// validation
			uint32 entsCnt = manager->getAllEntities()->entitiesCount();
			CAGE_TEST(entsCnt == reference.size());
			if (entsCnt == 0)
				continue;
			std::vector<entityClass*> allEntities;
			allEntities.reserve(entsCnt);
			std::vector<std::vector<entityClass*> > componentEntities;
			componentEntities.resize(totalComponents);
			for (referenceType::iterator it = reference.begin(), et = reference.end(); it != et; it++)
			{
				entityClass *e = manager->getEntity(it->first);
				allEntities.push_back(e);
				for (std::set<uint32>::iterator cit = it->second.begin(), cet = it->second.end(); cit != cet; cit++)
					componentEntities[*cit].push_back(e);
			}
			entityClass *const *entsBufConst = manager->getAllEntities()->entitiesArray();
			std::vector<entityClass*> entsBuf(entsBufConst, entsBufConst + entsCnt);
			std::sort(entsBuf.begin(), entsBuf.end());
			std::sort(allEntities.begin(), allEntities.end());
			CAGE_TEST(detail::memcmp(&entsBuf[0], &allEntities[0], sizeof(entityClass*) * entsCnt) == 0);
			for (uint32 i = 0; i < totalComponents; i++)
			{
				entsCnt = manager->getComponentByIndex(i)->getComponentEntities()->entitiesCount();
				CAGE_TEST(entsCnt == componentEntities[i].size());
				if (entsCnt == 0)
					continue;
				entsBufConst = manager->getComponentByIndex(i)->getComponentEntities()->entitiesArray();
				entsBuf = std::vector<entityClass*>(entsBufConst, entsBufConst + entsCnt);
				std::sort(entsBuf.begin(), entsBuf.end());
				std::sort(componentEntities[i].begin(), componentEntities[i].end());
				CAGE_TEST(detail::memcmp(&entsBuf[0], &componentEntities[i][0], sizeof(entityClass*) * entsCnt) == 0);
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

		holder<entityManagerClass> manager = newEntityManager(entityManagerCreateConfig());
		componentClass *components[totalComponents];
		groupClass *groups[totalGroups];

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
			entityClass *e = manager->newEntity(n);
			for (uint32 j = n % totalComponents, je = min(j + usedComponents, totalComponents); j < je; j++)
				e->addComponent(components[j]);
			for (uint32 j = 0; j < totalGroups; j++)
				if ((n + j) % totalGroups < usedGroups)
					e->addGroup(groups[j]);
			exists[n] = true;
		}

		holder <timerClass> tmr = newTimer();

		for (uint32 cycle = 0; cycle < totalCycles; cycle++)
		{
			if (cage::random() < 0.2)
			{ // simulate update
				// remove some entities
				for (uint32 i = 0, et = random(1, 20); i < et; i++)
				{
					uint32 n = random(random(1, entityNameIndex), entityNameIndex);
					if (exists[n])
					{
						exists[n] = false;
						manager->getEntity(n)->destroy();
					}
				}
				// add some new entities
				for (uint32 i = 0, et = random(1, 20); i < et; i++)
				{
					uint32 n = entityNameIndex++;
					entityClass *e = manager->newEntity(n);
					for (uint32 j = n % totalComponents, je = min(j + usedComponents, totalComponents); j < je; j++)
						e->addComponent(components[j]);
					for (uint32 j = 0; j < totalGroups; j++)
						if ((n + j) % totalGroups < usedGroups)
							e->addGroup(groups[j]);
					exists[n] = true;
				}
			}
			else
			{ // simulate draw
				for (uint32 j = 0; j < totalGroups; j++)
				{
					if (cage::random() < 0.5)
						continue;
					uint32 cnt = groups[j]->entitiesCount();
					entityClass *const *ents = groups[j]->entitiesArray();
					bool w = cage::random() < 0.2;
					for (uint32 k = 0; k < cnt; k++)
					{
						entityClass *e = ents[k];
						uint32 n = e->getName();
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
