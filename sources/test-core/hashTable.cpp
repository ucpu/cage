#include <map>
#include <unordered_map>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/hashTable.h>
#include <cage-core/timer.h>

namespace
{
	template<uint32 I, uint32 L>
	void performance()
	{
		holder<timer> tmr = newTimer();
		uint64 addStdMap, addStdUnordered, addCage;
		uint64 readStdMap, readStdUnordered, readCage;
		volatile void *res;

		{
			std::map<uint32, void*> mp;
			tmr->reset();
			for (uint32 i = 0; i <= I; i++)
				mp[i] = (void*)(uintPtr)(i);
			addStdMap = tmr->microsSinceLast();
			for (uint32 i = 0; i <= L; i++)
				res = mp[i % I];
			readStdMap = tmr->microsSinceLast();
		}

		{
			std::unordered_map<uint32, void*> mp;
			tmr->reset();
			for (uint32 i = 0; i <= I; i++)
				mp[i] = (void*)(uintPtr)(i);
			addStdUnordered = tmr->microsSinceLast();
			for (uint32 i = 0; i <= L; i++)
				res = mp[i % I];
			readStdUnordered = tmr->microsSinceLast();
		}

		{
			holder<hashTable<void>> tbl = newHashTable<void>({});
			tmr->reset();
			for (uint32 i = 0; i <= I; i++)
				tbl->add(i, (void*)(uintPtr)i);
			addCage = tmr->microsSinceLast();
			for (uint32 i = 0; i <= L; i++)
				res = tbl->get(i % I);
			readCage = tmr->microsSinceLast();
		}

		uint64 addStdBetter = min(addStdUnordered, addStdMap);
		uint64 readStdBetter = min(readStdUnordered, readStdMap);
		CAGE_LOG(severityEnum::Info, "performance", string("performance (add ") + I + "):\t\tstd::map: " + addStdMap + "\tstd::unordered: " + addStdUnordered + "\tcage: " + addCage + "\tratio: " + ((real)addCage / addStdBetter) + "\t" + (addCage < addStdBetter ? "better" : "worse"));
		CAGE_LOG(severityEnum::Info, "performance", string("performance (read ") + L + "):\tstd::map: " + readStdMap + "\tstd::unordered: " + readStdUnordered + "\tcage: " + readCage + "\tratio: " + ((real)readCage / readStdBetter) + "\t" + (readCage < readStdBetter ? "better" : "worse"));
	}
}

void testHashTable()
{
	CAGE_TESTCASE("hash table");

	{
		CAGE_TESTCASE("basic inserts and gets");
		holder<hashTable<void>> tbl = newHashTable<void>({});
		for (uint32 i = 0; i < 10000; i++)
			tbl->add(i, (void*)(uintPtr)i);
		for (uint32 i = 0; i < 10000; i++)
			CAGE_TEST(numeric_cast<uint32>((uintPtr)tbl->get(i)) == i);
		tbl->add(m, &tbl);
	}

	{
		CAGE_TESTCASE("more operations");
		holder<hashTable<void>> tbl = newHashTable<void>({});
		for (uint32 i = 0; i < 10000; i++)
			tbl->add(i, (void*)(uintPtr)i);
		for (uint32 i = 1000; i < 5000; i++)
			tbl->remove(i);
		tbl->remove(2000);
		CAGE_TEST(tbl->exists(500));
		CAGE_TEST(!tbl->exists(2000));
		CAGE_TEST(!tbl->exists(3000));
		CAGE_TEST(tbl->exists(7000));
		CAGE_TEST_THROWN(tbl->get(2000));
		CAGE_TEST(tbl->tryGet(2000) == nullptr);
		CAGE_TEST(tbl->get(500) == (void*)500);
		CAGE_TEST(tbl->tryGet(500) == (void*)500);
		CAGE_TEST(tbl->get(7000) == (void*)7000);
	}

	{
		CAGE_TESTCASE("randomized access");
		holder<hashTable<void>> tbl = newHashTable<void>({});
		std::map<uint32, void*> mp;
		for (uint32 i = 0; i < 100000; i++)
		{
			if (mp.size() == 5000 || (mp.size() > 0 && randomRange(0, 100) < 30))
			{
				uint32 name = mp.begin()->first;
				CAGE_TEST(tbl->get(name) == mp[name]);
				tbl->remove(name);
				mp.erase(name);
			}
			else
			{
				uint32 name = randomRange(0, 100000);
				if (mp.find(name) != mp.end())
				{
					CAGE_TEST(tbl->get(name) == mp[name]);
					tbl->remove(name);
				}
				void *value = (void*)numeric_cast<uintPtr>(randomRange(0, 100000));
				mp[name] = value;
				tbl->add(name, (void*)(uintPtr)value);
			}
			CAGE_TEST(mp.size() == tbl->count());
		}
		for (const auto &it : mp)
		{
			CAGE_TEST(tbl->get(it.first) == it.second);
			tbl->remove(it.first);
		}
		CAGE_TEST(tbl->count() == 0);
	}

	{
		CAGE_TESTCASE("templates");
		holder<hashTable<uint32>> tbl = newHashTable<uint32>({});
		uint32 i = 42;
		tbl->add(13, &i);
		CAGE_TEST(tbl->get(13) == &i);
		CAGE_TEST(tbl->count() == 1);
	}

	{
		CAGE_TESTCASE("iterators");
		holder<hashTable<uint32>> tbl = newHashTable<uint32>({});
		CAGE_TEST(tbl->begin() == tbl->end());
		uint32 i = 42;
		tbl->add(13, &i);
		tbl->add(42, &i);
		uint32 c = 0;
		for (const auto &it : *tbl)
		{
			c++;
			CAGE_TEST(it.first == 13 || it.first == 42);
			CAGE_TEST(it.second == &i);
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
		CAGE_TEST(hash(42) == hash(42));
	}
}
