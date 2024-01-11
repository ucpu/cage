#include "main.h"

#include <cage-core/flatBag.h>
#include <cage-core/flatSet.h>
#include <cage-core/math.h>
#include <cage-core/stdHash.h>

void testFlatBag()
{
	CAGE_TESTCASE("flatBag");

	{
		CAGE_TESTCASE("basics");

		FlatBag<uint32> s;
		s.insert(13);
		s.insert(1024);
		s.insert(42);
		CAGE_TEST(s.size() == 3);
		s.insert(42);
		CAGE_TEST(s.size() == 3);
		CAGE_TEST(s.count(20) == 0);
		CAGE_TEST(s.count(42) == 1);
		CAGE_TEST(!s.empty());
		s.erase(13);
		CAGE_TEST(s.size() == 2);
		s.erase(13);
		CAGE_TEST(s.size() == 2);
		s.clear();
		CAGE_TEST(s.size() == 0);
		CAGE_TEST(s.empty());
	}

	{
		CAGE_TESTCASE("const bag");

		const FlatBag<uint32> s = []()
		{
			FlatBag<uint32> s;
			s.insert(13);
			s.insert(42);
			return s;
		}();
		CAGE_TEST(s.count(20) == 0);
		CAGE_TEST(s.count(42) == 1);
		CAGE_TEST(s.size() == 2);
		CAGE_TEST(!s.empty());
	}

	{
		CAGE_TESTCASE("access FlatBag as PointerRange");

		FlatBag<uint32> s;
		s.insert(1024);
		s.insert(13);
		s.insert(42);
		PointerRange<const uint32> pr = s;
		CAGE_TEST(pr.size() == 3);
	}

	{
		CAGE_TESTCASE("find");

		FlatBag<uint32> s;
		s.insert(1024);
		s.insert(13);
		s.insert(42);
		CAGE_TEST(s.find(13) != s.end());
		CAGE_TEST(s.find(20) == s.end());
	}

	{
		CAGE_TESTCASE("erase");

		FlatBag<uint32> s;
		s.insert(1024);
		s.insert(13);
		s.insert(42);
		CAGE_TEST(s.size() == 3);
		s.erase(42);
		CAGE_TEST(s.size() == 2);
		CAGE_TEST(s.count(1024) == 1);
		CAGE_TEST(s.count(13) == 1);
		CAGE_TEST(s.count(42) == 0);
	}

	{
		CAGE_TESTCASE("randomized");

		FlatBag<uint32> b;
		FlatSet<uint32> s; // reference implementation

		for (uint32 round = 0; round < 10000; round++)
		{
			const uint32 i = randomRange(13, 420);
			if (s.count(i))
			{
				CAGE_TEST(b.count(i));
				s.erase(i);
				b.erase(i);
			}
			else
			{
				CAGE_TEST(!b.count(i));
				s.insert(i);
				b.insert(i);
			}
			CAGE_TEST(s.size() == b.size());
			const FlatSet<uint32> tmp = makeFlatSet<uint32>(b.unsafeData());
			CAGE_TEST(tmp == s);
		}
	}
}
