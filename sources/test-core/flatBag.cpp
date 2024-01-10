#include "main.h"

#include <cage-core/flatBag.h>
#include <cage-core/flatSet.h>
#include <cage-core/math.h>

namespace
{
	constexpr uint32 testConstexpr()
	{
		FlatBag<String> s;
		s.insert("hello");
		s.insert("there");
		s.insert("Obi-Wan");
		s.insert("Kenobi");
		s.insert("says");
		s.insert("how");
		s.insert("are");
		s.insert("you");
		s.erase("bro");
		return s.size();
	}
}

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
		CAGE_TEST_ASSERTED(s.insert(1024));
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
		CAGE_TEST_ASSERTED(s.insert(1024));
		PointerRange<const uint32> pr = s;
		CAGE_TEST(pr.size() == 3);
	}

	{
		CAGE_TESTCASE("custom struct");

		struct Custom
		{
			uint32 v = m;
			constexpr explicit Custom(uint32 v) : v(v) {}
			constexpr bool operator<(const Custom &b) const noexcept { return v < b.v; }
			constexpr bool operator==(const Custom &b) const noexcept { return v == b.v; }
		};

		FlatBag<Custom> s;
		s.insert(Custom(1024));
		s.insert(Custom(13));
		s.insert(Custom(42));
		CAGE_TEST(s.size() == 3);
		CAGE_TEST(s.count(Custom(20)) == 0);
		CAGE_TEST(s.count(Custom(42)) == 1);
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
		CAGE_TESTCASE("constexpr");
		constexpr const uint32 a = testConstexpr();
		uint32 b = testConstexpr();
		CAGE_TEST(a == b);
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
