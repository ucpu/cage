#include "main.h"
#include <cage-core/enumerate.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/flatSet.h>

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

	Holder<PointerRange<const string>> genRange()
	{
		return PointerRangeHolder<const string>(genStrings());
	}

	std::vector<Holder<string>> genHolders()
	{
		std::vector<Holder<string>> res;
		res.push_back(systemArena().createHolder<string>("ar"));
		res.push_back(systemArena().createHolder<string>("ma"));
		res.push_back({});
		res.push_back(systemArena().createHolder<string>("ge"));
		res.push_back(systemArena().createHolder<string>("don"));
		return res;
	}

	Holder<PointerRange<Holder<PointerRange<const string>>>> genRangeOfRanges()
	{
		PointerRangeHolder<Holder<PointerRange<const string>>> vec;
		vec.push_back(genRange());
		vec.push_back(genRange());
		vec.push_back(genRange());
		vec.push_back(genRange());
		return vec;
	}

	constexpr auto constexprTestArray()
	{
		constexpr const int arr[] = { 42, -1, 3 };
		int res = 0;
		for (const auto &it : enumerate(arr))
			res += (int)it.index * *it;
		return res;
	}

	constexpr auto constexprTestIterator()
	{
		constexpr const int arr[] = { 42, -1, 3 };
		int res = 0;
		for (const auto &it : enumerate(std::begin(arr), std::end(arr)))
			res += (int)it.index * *it;
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
			CAGE_TEST(it.index == i++);
			CAGE_TEST(*it == names[it.index]);
			CAGE_TEST(it->size() == names[it.index].size());
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("iterators with offset");
		uint32 i = 0;
		for (const auto &it : enumerate(names.begin(), names.end(), 5))
		{
			CAGE_TEST(*it == names[i]);
			CAGE_TEST(it.index == i++ + 5);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("basic vector");
		uint32 i = 0;
		for (const auto &it : enumerate(names))
		{
			CAGE_TEST(it.index == i++);
			CAGE_TEST(*it == names[it.index]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("temporary range");
		auto range = enumerate(genStrings());
		uint32 i = 0;
		for (const auto &it : range)
			CAGE_TEST(it.index == i++);
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("temporary vector");
		uint32 i = 0;
		for (const auto &it : enumerate(genStrings()))
			CAGE_TEST(it.index == i++);
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("const vector");
		const std::vector<string> cns(names);
		uint32 i = 0;
		for (const auto &it : enumerate(cns))
		{
			CAGE_TEST(it.index == i++);
			CAGE_TEST(*it == names[it.index]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("c array");
		const char *arr[] = { "a", "bb", "ccc" };
		uint32 i = 0;
		for (const auto &it : enumerate(arr))
		{
			CAGE_TEST(it.index == i++);
			CAGE_TEST(*it == arr[it.index]);
		}
		CAGE_TEST(i == 3);
	}

	{
		CAGE_TESTCASE("pointer range");
		uint32 i = 0;
		for (const auto &it : enumerate(PointerRange<const string>(names)))
		{
			CAGE_TEST(it.index == i++);
			CAGE_TEST(*it == names[it.index]);
		}
		CAGE_TEST(i == names.size());
	}

	{
		CAGE_TESTCASE("holder of pointer range");
		uint32 i = 0;
		for (const auto &it : enumerate(genRange()))
		{
			CAGE_TEST(it.index == i++);
			CAGE_TEST(it->length() > 0);
		}
		CAGE_TEST(i == genStrings().size());
	}

	{
		CAGE_TESTCASE("vector of holders");
		uint32 i = 0;
		for (const auto &it : enumerate(genHolders()))
		{
			// unfortunately, we cannot use * (dereference operator), because that would attempt to create a copy of the Holder
			CAGE_TEST((!!it->get()) == (i != 2)); // test that the element at index 2 is empty Holder
			CAGE_TEST(it.index == i++);
		}
		CAGE_TEST(i == genHolders().size());
	}

	{
		CAGE_TESTCASE("range of ranges 1");
		uint32 i = 0;
		for (const auto &it : enumerate(genRangeOfRanges()))
		{
			CAGE_TEST(it->size() == 4);
			CAGE_TEST(it.index == i++);
		}
		CAGE_TEST(i == genRangeOfRanges().size());
	}

	{
		CAGE_TESTCASE("range of ranges 2");
		const auto range = genRangeOfRanges();
		uint32 i = 0;
		for (const auto &it : enumerate(range))
		{
			CAGE_TEST(it->size() == 4);
			CAGE_TEST(it.index == i++);
		}
		CAGE_TEST(i == range.size());
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
			CAGE_TEST(it.index == i++);
			test[it->first] = it->second;
		}
		CAGE_TEST(i == orig.size());
		CAGE_TEST(test == orig);
		CAGE_TEST(orig.size() == 4);
	}

	{
		CAGE_TESTCASE("modifying values");
		std::vector<int> vec;
		vec.push_back(13);
		vec.push_back(42);
		for (const auto &it : enumerate(vec))
			it.get()++;
		CAGE_TEST(vec[0] == 14);
		CAGE_TEST(vec[1] == 43);
	}

	{
		CAGE_TESTCASE("flat set");
		FlatSet<uint32> fs;
		fs.insert(13);
		fs.insert(42);
		uint32 i = 0;
		for (const auto &it : enumerate(fs))
		{
			CAGE_TEST(it.index == i++);
		}
		CAGE_TEST(i == fs.size());
	}

	{
		CAGE_TESTCASE("constexpr enumerate array");
		constexpr auto index = constexprTestArray();
		CAGE_TEST(index == 5);
	}

	{
		CAGE_TESTCASE("constexpr enumerate iterators");
		constexpr auto index = constexprTestIterator();
		CAGE_TEST(index == 5);
	}
}
