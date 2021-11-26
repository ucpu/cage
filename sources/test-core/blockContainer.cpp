#include "main.h"
#include <cage-core/blockContainer.h>

#include <vector>

namespace
{
	template<class T>
	std::vector<T> flatten(const BlockContainer<T> &c)
	{
		std::vector<T> r;
		r.reserve(c.size());
		for (const auto &a : c)
			for (const auto &b : a)
				r.push_back(b);
		return r;
	}
}

void testBlockContainer()
{
	CAGE_TESTCASE("block container");

	{
		CAGE_TESTCASE("basics");
		BlockContainer<uint32> cont(3);
		CAGE_TEST(cont.empty());
		CAGE_TEST(cont.size() == 0);
		cont.push_back(13);
		CAGE_TEST(!cont.empty());
		CAGE_TEST(cont.size() == 1);
		cont.push_back(42);
		cont.push_back(7);
		cont.push_back(8);
		CAGE_TEST(!cont.empty());
		CAGE_TEST(cont.size() == 4);
		CAGE_TEST((*cont.begin()).size() == 3);
		CAGE_TEST((flatten(cont) == std::vector<uint32>{ 13, 42, 7, 8 }));
	}
}
