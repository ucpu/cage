#include "main.h"
#include <cage-core/lruCache.h>

namespace
{
	struct Key
	{
		uint32 k;
		Key(uint32 k = 0) : k(k) {}
	};

	struct Hasher
	{
		auto operator () (const Key &k) const
		{
			return std::hash<uint32>()(k.k);
		}
	};

	bool operator == (const Key &a, const Key &b)
	{
		return a.k == b.k;
	}

	struct Value
	{
		uint32 v;
		Value(uint32 v = 0) : v(v) {}
	};
}

void testLruCache()
{
	CAGE_TESTCASE("LruCache");

	{
		CAGE_TESTCASE("basics");
		LruCache<uint32, uint32> cache(3);
		CAGE_TEST(!cache.find(13));
		uint32 setResult = cache.set(13, 42);
		std::optional<uint32> findResult = cache.find(13);
		CAGE_TEST(findResult);
		CAGE_TEST(*findResult == 42);
		cache.clear();
		CAGE_TEST(!cache.find(13));
		CAGE_TEST(*findResult == 42);
	}

	{
		CAGE_TESTCASE("with custom types");
		LruCache<Key, Value, Hasher> cache(3);
		CAGE_TEST(!cache.find(13));
		Value setResult = cache.set(13, 42);
		std::optional<Value> findResult = cache.find(13);
		CAGE_TEST(findResult);
		CAGE_TEST(findResult->v == 42);
		cache.clear();
		CAGE_TEST(!cache.find(13));
		CAGE_TEST(findResult->v == 42);
	}

	{
		CAGE_TESTCASE("with holder");
		LruCache<uint32, Holder<uint32>> cache(3);
		CAGE_TEST(!cache.find(13));
		Holder<uint32> setResult = cache.set(13, systemMemory().createHolder<uint32>(42));
		Holder<uint32> findResult = cache.find(13);
		CAGE_TEST(findResult);
		CAGE_TEST(*findResult == 42);
		cache.clear();
		CAGE_TEST(!cache.find(13));
		CAGE_TEST(*findResult == 42);
	}

	{
		CAGE_TESTCASE("deleting in order");
		LruCache<uint32, uint32> cache(3);
		cache.set(1, 1);
		cache.set(2, 2);
		cache.set(3, 3);
		CAGE_TEST(cache.find(1));
		CAGE_TEST(cache.find(2));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(!cache.find(5));
		cache.set(4, 4); // deletes 1
		CAGE_TEST(!cache.find(1));
		CAGE_TEST(cache.find(2));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(cache.find(4));
		CAGE_TEST(!cache.find(5));
		cache.set(5, 5); // deletes 2
		CAGE_TEST(!cache.find(1));
		CAGE_TEST(!cache.find(2));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(cache.find(4));
		CAGE_TEST(cache.find(5));
	}

	{
		CAGE_TESTCASE("deleting in reverse order");
		LruCache<uint32, uint32> cache(3);
		cache.set(1, 1);
		cache.set(2, 2);
		cache.set(3, 3);
		CAGE_TEST(!cache.find(5));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(cache.find(2));
		CAGE_TEST(cache.find(1));
		cache.set(4, 4); // deletes 3 (least recently used, not set)
		CAGE_TEST(!cache.find(5));
		CAGE_TEST(cache.find(4));
		CAGE_TEST(!cache.find(3));
		CAGE_TEST(cache.find(2));
		CAGE_TEST(cache.find(1));
		cache.set(5, 5); // deletes 4 (least recently used, not set)
		CAGE_TEST(cache.find(5));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(!cache.find(3));
		CAGE_TEST(cache.find(2));
		CAGE_TEST(cache.find(1));
	}

	{
		CAGE_TESTCASE("erase");
		LruCache<uint32, uint32> cache(3);
		cache.set(1, 1);
		cache.set(2, 2);
		cache.set(3, 3);
		CAGE_TEST(cache.find(1));
		CAGE_TEST(cache.find(2));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(!cache.find(5));
		cache.erase(2);
		cache.set(4, 4);
		CAGE_TEST(cache.find(1));
		CAGE_TEST(!cache.find(2));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(cache.find(4));
		CAGE_TEST(!cache.find(5));
		cache.erase(4);
		CAGE_TEST(cache.find(1));
		CAGE_TEST(!cache.find(2));
		CAGE_TEST(cache.find(3));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(!cache.find(5));
		cache.erase(1);
		cache.erase(3);
		cache.erase(13); // test erasing non-existent item
		CAGE_TEST(!cache.find(1));
		CAGE_TEST(!cache.find(2));
		CAGE_TEST(!cache.find(3));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(!cache.find(5));
		cache.set(5, 5);
		CAGE_TEST(!cache.find(1));
		CAGE_TEST(!cache.find(2));
		CAGE_TEST(!cache.find(3));
		CAGE_TEST(!cache.find(4));
		CAGE_TEST(cache.find(5));
	}
}
