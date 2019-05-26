#include <vector>
#include <initializer_list>

#include "main.h"
#include <cage-core/enumerate.h>

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
	}

	{
		CAGE_TESTCASE("iterators with offset");
		uint32 i = 0;
		for (auto it : enumerate(names.begin(), names.end(), 5))
		{
			CAGE_TEST(*it == names[i]);
			CAGE_TEST(it.cnt == i++ + 5);
		}
	}

	{
		CAGE_TESTCASE("basic vector");
		uint32 i = 0;
		for (auto it : enumerate(names))
		{
			CAGE_TEST(it.cnt == i++);
			CAGE_TEST(*it == names[it.cnt]);
		}
	}

	{
		CAGE_TESTCASE("temporary range");
		auto range = enumerate(genStrings());
		uint32 i = 0;
		for (auto it : range)
			CAGE_TEST(it.cnt == i++);
		CAGE_TEST(i == 4);
	}

	{
		CAGE_TESTCASE("temporary vector");
		uint32 i = 0;
		for (auto it : enumerate(genStrings()))
			CAGE_TEST(it.cnt == i++);
		CAGE_TEST(i == 4);
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
	}

	/*
	{
		CAGE_TESTCASE("initializer list");

		uint32 i = 5;
		for (auto it : enumerate({ "a", "b", "c" }))
		{
			// todo
		}
	}
	*/
}
