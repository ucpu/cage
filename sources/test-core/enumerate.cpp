#include <vector>

#include "main.h"
#include <cage-core/enumerate.h>
#include <cage-core/pointerRangeHolder.h>

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

	holder<pointerRange<const string>> genHolder()
	{
		return pointerRangeHolder<const string>(genStrings());
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
		for (auto it : enumerate(names.begin(), names.end()))
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
		for (auto it : enumerate(names.begin(), names.end(), 5))
		{
			CAGE_TEST(*it == names[i]);
			CAGE_TEST(it.cnt == i++ + 5);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("basic vector");
		uint32 i = 0;
		for (auto it : enumerate(names))
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
		for (auto it : range)
			CAGE_TEST(it.cnt == i++);
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("temporary vector");
		uint32 i = 0;
		for (auto it : enumerate(genStrings()))
			CAGE_TEST(it.cnt == i++);
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("const vector");
		const std::vector<string> cns(names);
		uint32 i = 0;
		for (auto it : enumerate(cns))
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
		for (auto it : enumerate(arr))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == arr[it.cnt]);
		}
		CAGE_TEST(i == 3);
	}

	{
		CAGE_TESTCASE("pointer range");
		uint32 i = 0;
		for (auto it : enumerate(pointerRange<const string>(names)))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == names[it.cnt]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("holder of pointer range");
		uint32 i = 0;
		for (auto it : enumerate(genHolder()))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(it->length() > 0);
		}
		CAGE_TEST(i == genStrings().size());
	}
}
