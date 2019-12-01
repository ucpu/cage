#include "main.h"
#include <cage-core/enumerate.h>
#include <cage-core/pointerRangeHolder.h>

#include <vector>
#include <map>

namespace
{
	std::vector<string> genStrings()
	{
		std::vector<string> res;
		res.push_back("ar");
		res.push_back("ma");
		res.push_back("ge");
		res.push_back("don");
		return res;
	}

	holder<pointerRange<const string>> genRange()
	{
		return pointerRangeHolder<const string>(genStrings());
	}

	std::vector<holder<string>> genHolders()
	{
		std::vector<holder<string>> res;
		res.push_back(detail::systemArena().createHolder<string>("ar"));
		res.push_back(detail::systemArena().createHolder<string>("ma"));
		res.push_back({});
		res.push_back(detail::systemArena().createHolder<string>("ge"));
		res.push_back(detail::systemArena().createHolder<string>("don"));
		return res;
	}
}

void testEnumerate()
{
	CAGE_TESTCASE("enumerate");

	std::vector<string> names;
	names.push_back("Emma");
	names.push_back("Olivia");
	names.push_back("Ava");
	names.push_back("Isabella");
	names.push_back("Sophia");

	{
		CAGE_TESTCASE("direct iterators");
		uint32 i = 0;
		for (const auto &it : enumerate(names.begin(), names.end()))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == names[it.cnt]);
			CAGE_TEST(it->size() == names[it.cnt].size());
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("iterators with offset");
		uint32 i = 0;
		for (const auto &it : enumerate(names.begin(), names.end(), 5))
		{
			CAGE_TEST(*it == names[i]);
			CAGE_TEST(it.cnt == i++ + 5);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("basic vector");
		uint32 i = 0;
		for (const auto &it : enumerate(names))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == names[it.cnt]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("temporary range");
		auto range = enumerate(genStrings());
		uint32 i = 0;
		for (const auto &it : range)
			CAGE_TEST(it.cnt == i++);
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("temporary vector");
		uint32 i = 0;
		for (const auto &it : enumerate(genStrings()))
			CAGE_TEST(it.cnt == i++);
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("const vector");
		const std::vector<string> cns(names);
		uint32 i = 0;
		for (const auto &it : enumerate(cns))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == names[it.cnt]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("c array");
		const char *arr[] = { "a", "bb", "ccc" };
		uint32 i = 0;
		for (const auto &it : enumerate(arr))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == arr[it.cnt]);
		}
		CAGE_TEST(i == 3);
	}

	{
		CAGE_TESTCASE("pointer range");
		uint32 i = 0;
		for (const auto &it : enumerate(pointerRange<const string>(names)))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == names[it.cnt]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("holder of pointer range");
		uint32 i = 0;
		for (const auto &it : enumerate(genRange()))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(it->length() > 0);
		}
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("vector of holders");
		uint32 i = 0;
		for (const auto &it : enumerate(genHolders()))
		{
			// unfortunately, we cannot use * (dereference operator), because that would attempt to create a copy of the holder
			CAGE_TEST((!!it->get()) == (i != 2)); // test that the element at index 2 is empty holder
			CAGE_TEST(it.cnt == i++);
		}
		CAGE_TEST(i == genHolders().size());
	}

	{
		CAGE_TESTCASE("map");
		std::map<string, string> orig;
		orig["a"] = "a";
		orig["b"] = "b";
		orig["hello"] = "world";
		orig["duck"] = "quack";
		std::map<string, string> test;
		uint32 i = 0;
		for (const auto &it : enumerate(orig))
		{
			CAGE_TEST(it.cnt == i++);
			test[it->first] = it->second;
		}
		CAGE_TEST(i == orig.size());
		CAGE_TEST(test == orig);
		CAGE_TEST(orig.size() == 4);
	}
}
