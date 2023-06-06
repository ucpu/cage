#include "main.h"

#include <cage-core/flatSet.h>

void testFlatSet()
{
	CAGE_TESTCASE("flatSet");

	{
		CAGE_TESTCASE("basics");

		FlatSet<uint32> s;
		s.insert(13);
		s.insert(1024);
		CAGE_TEST(s.insert(42).second == true);
		CAGE_TEST(s.size() == 3);
		CAGE_TEST(s.insert(42).second == false);
		CAGE_TEST(s.size() == 3);
		CAGE_TEST(s.count(20) == 0);
		CAGE_TEST(s.count(42) == 1);
		CAGE_TEST(!s.empty());
		CAGE_TEST(s.data()[0] == 13);
		CAGE_TEST(s.data()[1] == 42);
		CAGE_TEST(s.data()[2] == 1024);
		s.erase(13);
		CAGE_TEST(s.size() == 2);
		s.erase(13);
		CAGE_TEST(s.size() == 2);
		s.clear();
		CAGE_TEST(s.size() == 0);
		CAGE_TEST(s.empty());
	}

	{
		CAGE_TESTCASE("const set");

		const FlatSet<uint32> s = []()
		{
			FlatSet<uint32> s;
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
		CAGE_TESTCASE("access FlatSet as PointerRange");

		FlatSet<uint32> s;
		s.insert(1024);
		s.insert(13);
		s.insert(42);
		s.insert(1024);
		PointerRange<const uint32> pr = s;
		CAGE_TEST(pr.size() == 3);
	}

	{
		CAGE_TESTCASE("custom struct and custom comparator");

		struct Custom
		{
			uint32 v = m;

			explicit Custom(uint32 v) : v(v) {}
		};

		struct Comparator
		{
			bool operator()(const Custom &a, const Custom &b) const { return a.v < b.v; }
		};

		FlatSet<Custom, Comparator> s;
		s.insert(Custom(1024));
		s.insert(Custom(13));
		s.insert(Custom(42));
		s.insert(Custom(1024));
		CAGE_TEST(s.size() == 3);
		CAGE_TEST(s.count(Custom(20)) == 0);
		CAGE_TEST(s.count(Custom(42)) == 1);
	}

	{
		CAGE_TESTCASE("custom struct and default comparator");

		struct Custom
		{
			uint32 v = m;

			explicit Custom(uint32 v) : v(v) {}

			bool operator<(const Custom &b) const { return v < b.v; }
		};

		FlatSet<Custom> s;
		s.insert(Custom(1024));
		s.insert(Custom(13));
		s.insert(Custom(42));
		s.insert(Custom(1024));
		CAGE_TEST(s.size() == 3);
		CAGE_TEST(s.count(Custom(20)) == 0);
		CAGE_TEST(s.count(Custom(42)) == 1);
	}

	{
		CAGE_TESTCASE("insert range");

		std::vector<uint32> vec;
		vec.push_back(1024);
		vec.push_back(13);
		vec.push_back(42);
		vec.push_back(1024);
		FlatSet<uint32> s;
		s.insert(vec.begin(), vec.end());
		CAGE_TEST(s.size() == 3);
	}

	{
		CAGE_TESTCASE("insert with initializer list");

		FlatSet<uint32> s;
		s.insert({ 1024, 13, 42, 1024 });
		CAGE_TEST(s.size() == 3);
	}

	{
		CAGE_TESTCASE("init with range");

		std::vector<uint32> vec;
		vec.push_back(1024);
		vec.push_back(13);
		vec.push_back(42);
		vec.push_back(1024);
		FlatSet<uint32> s(vec.begin(), vec.end());
		CAGE_TEST(s.size() == 3);
	}

	{
		CAGE_TESTCASE("init with initializer list");

		FlatSet<uint32> s({ 1024, 13, 42, 1024 });
		CAGE_TEST(s.size() == 3);
	}

	{
		CAGE_TESTCASE("operator ==");

		FlatSet<uint32> s1({ 1, 3, 7, 9 });
		FlatSet<uint32> s2({ 9, 7, 1, 3 });
		FlatSet<uint32> s3({ 2, 4, 6, 8 });
		CAGE_TEST(s1 == s2);
		CAGE_TEST(!(s1 == s3));
	}

	{
		CAGE_TESTCASE("find");

		FlatSet<uint32> s({ 1024, 13, 42, 1024 });
		CAGE_TEST(s.find(13) == s.begin());
		CAGE_TEST(s.find(20) == s.end());
	}

	{
		CAGE_TESTCASE("erase");

		FlatSet<uint32> s({ 1024, 13, 42, 1024 });
		s.erase(s.begin() + 1, s.begin() + 2);
		CAGE_TEST(s.size() == 2);
		CAGE_TEST(s == FlatSet<uint32>({ 13, 1024 }));
	}
}
