#include <map>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/hashTable.h>
#include <cage-core/timer.h>

template<uint32 I, uint32 L> void performance()
{
	holder<timerClass> tmr = newTimer();
	uint32 tm_system, tm_hash;
	volatile void *res;

	{
		std::map<uint32, void*> mp;
		for (uint32 i = 1; i <= I; i++)
			mp[i] = (void*)(uintPtr)(i);
		tmr->reset();
		for (uint32 i = 1; i <= L; i++)
			res = mp[i % I + 1];
		tm_system = numeric_cast<uint32>(tmr->microsSinceStart() / 100.0);
	}

	{
		holder<hashTableClass<void>> tbl = newHashTable<void>(100, 100000);
		for (uint32 i = 1; i <= I; i++)
			tbl->add(i, (void*)(uintPtr)i);
		tmr->reset();
		for (uint32 i = 1; i <= L; i++)
			res = tbl->get(i % I + 1, false);
		tm_hash = numeric_cast<uint32>(tmr->microsSinceStart() / 100.0);
	}

	CAGE_LOG(severityEnum::Info, "performance", string("performance (") + I + "):\t" + tm_system + "\t" + tm_hash + "\t" + ((real)tm_hash / tm_system) + "\t" + (tm_hash < tm_system ? "ok" : "bad"));
}

void testHashTable()
{
	CAGE_TESTCASE("hash table");

	{
		CAGE_TESTCASE("basic inserts and gets");
		holder<hashTableClass<void>> tbl = newHashTable<void>(10, 20000);
		for (uint32 i = 1; i < 10000; i++)
			tbl->add(i, (void*)(uintPtr)i);
		for (uint32 i = 1; i < 10000; i++)
			CAGE_TEST(numeric_cast<uint32>(tbl->get(i, false)) == i);
	}

	{
		CAGE_TESTCASE("more operations");
		holder<hashTableClass<void>> tbl = newHashTable<void>(10, 20000);
		for (uint32 i = 1; i < 10000; i++)
			tbl->add(i, (void*)(uintPtr)i);
		for (uint32 i = 1000; i < 5000; i++)
			tbl->remove(i);
		tbl->remove(2000);
		CAGE_TEST(tbl->exists(500));
		CAGE_TEST(!tbl->exists(2000));
		CAGE_TEST(!tbl->exists(3000));
		CAGE_TEST(tbl->exists(7000));
		CAGE_TEST_THROWN(tbl->get(2000, false));
		CAGE_TEST(tbl->get(500, false) == (void*)500);
		CAGE_TEST(tbl->get(7000, true) == (void*)7000);
	}

	{
		CAGE_TESTCASE("randomized access");
		holder<hashTableClass<void>> tbl = newHashTable<void>(100, 20000);
		std::map<uint32, uint32> mp;
		for (uint32 i = 0; i < 100000; i++)
		{
			if (mp.size() == 5000 || (mp.size() > 0 && random(0, 100) < 30))
			{
				uint32 name = mp.begin()->first;
				CAGE_TEST(numeric_cast<uint32>(tbl->get(name, false)) == mp[name]);
				tbl->remove(name);
				mp.erase(name);
			}
			else
			{
				uint32 name = random(1, 100000);
				if (mp.find(name) != mp.end())
				{
					CAGE_TEST(numeric_cast<uint32>(tbl->get(name, false)) == mp[name]);
					tbl->remove(name);
				}
				uint32 value = random(1, 100000);
				mp[name] = value;
				tbl->add(name, (void*)(uintPtr)value);
			}
			CAGE_TEST(mp.size() == tbl->count());
		}
		for (std::map<uint32, uint32>::iterator it = mp.begin(), e = mp.end(); it != e; it++)
		{
			CAGE_TEST(numeric_cast<uint32>(tbl->get(it->first, false)) == it->second);
			tbl->remove(it->first);
		}
		CAGE_TEST(tbl->count() == 0);
	}

	{
		CAGE_TESTCASE("templates");
		holder<hashTableClass<uint32>> tbl = newHashTable<uint32>(100, 1000);
		uint32 i = 42;
		tbl->add(13, &i);
		CAGE_TEST(tbl->get(13, false) == &i);
		CAGE_TEST(tbl->count() == 1);
	}

	{
		CAGE_TESTCASE("iterators");
		holder<hashTableClass<uint32>> tbl = newHashTable<uint32>(100, 1000);
		uint32 i = 42;
		tbl->add(13, &i);
		tbl->add(42, &i);
		uint32 c = 0;
		for (auto it : *tbl)
        {
			c++;
			(void)it;
        }
		CAGE_TEST(c == 2);
	}

	{
		CAGE_TESTCASE("hash table performance");
#ifdef CAGE_DEBUG
#define Lookups 1000
#else
#define Lookups 1000000
#endif
		performance< 100, Lookups>();
		performance< 500, Lookups>();
		performance<1000, Lookups>();
		performance<5000, Lookups>();
	}

	{
		CAGE_TESTCASE("hash function");
		using detail::hash;
		CAGE_TEST(hash(13) != hash(15));
	}
}
