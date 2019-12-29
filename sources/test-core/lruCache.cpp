#include "main.h"
#include <cage-core/lruCache.h>

namespace
{
	struct key
	{
		uint32 k;
		key(uint32 k = 0) : k(k) {}
	};

	struct hasher
	{
		auto operator () (const key &k) const
		{
			return std::hash<uint32>()(k.k);
		}
	};

	bool operator == (const key &a, const key &b)
	{
		return a.k == b.k;
	}

	struct value
	{
		uint32 v;
		value(uint32 v = 0) : v(v) {}
	};
}

void testLruCache()
{
	CAGE_TESTCASE("LruCache");

	{
		CAGE_TESTCASE("basics");
		LruCache<uint32, uint32> cache(3);
		CAGE_TEST(cache.find(13) == nullptr);
		cache.set(13, 130);
		CAGE_TEST(cache.find(13));
		CAGE_TEST(*cache.find(13) == 130);
		cache.clear();
		CAGE_TEST(cache.find(13) == nullptr);
	}

	{
		CAGE_TESTCASE("with custom types");
		LruCache<key, value, hasher> cache(3);
		CAGE_TEST(cache.find(13) == nullptr);
		cache.set(13, 130);
		CAGE_TEST(cache.find(13));
		CAGE_TEST(cache.find(13)->v == 130);
		cache.clear();
		CAGE_TEST(cache.find(13) == nullptr);
	}

	{
		CAGE_TESTCASE("with holder");
		LruCache<uint32, Holder<uint32>> cache(3);
		CAGE_TEST(cache.find(13) == nullptr);
		cache.set(13, detail::systemArena().createHolder<uint32>(13));
		CAGE_TEST(cache.find(13));
		CAGE_TEST(**cache.find(13) == 13);
		cache.clear();
		CAGE_TEST(cache.find(13) == nullptr);
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
