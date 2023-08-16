#include <unordered_set>

#include "main.h"

#include <cage-core/stdHash.h>

namespace
{
	// item without std::hash specialization
	struct AnItem
	{};

	bool operator==(const AnItem &a, const AnItem &b) noexcept
	{
		return &a == &b;
	}
}

void testStdHash()
{
	CAGE_TESTCASE("stdHash");

	{
		CAGE_TESTCASE("string");
		CAGE_TEST(std::hash<String>()("hello") != std::hash<String>()("world"));
		std::unordered_set<String> s;
		s.insert("hello");
		s.insert("world");
	}

	{
		CAGE_TESTCASE("holder of string");
		auto a = systemMemory().createHolder<String>("hello");
		auto b = systemMemory().createHolder<String>("world");
		auto c = systemMemory().createHolder<String>("hello");
		std::hash<Holder<String>> h;
		CAGE_TEST(h(a) != h(b));
		CAGE_TEST(h(a) == h(c)); // compares hashes of the strings -> they are equal
		h({});
		std::unordered_set<Holder<String>> s;
		s.insert(a.share());
		s.insert(a.share());
		s.insert(b.share());
		s.insert(c.share());
		CAGE_TEST(s.size() == 3);
	}

	{
		CAGE_TESTCASE("holder of an item");
		auto a = systemMemory().createHolder<AnItem>();
		auto b = systemMemory().createHolder<AnItem>();
		auto c = systemMemory().createHolder<AnItem>();
		std::hash<Holder<AnItem>> h;
		CAGE_TEST(h(a) != h(b));
		CAGE_TEST(h(a) != h(c)); // compares hashes of the pointers -> they are different
		h({});
		std::unordered_set<Holder<AnItem>> s;
		s.insert(a.share());
		s.insert(a.share());
		s.insert(b.share());
		s.insert(c.share());
		CAGE_TEST(s.size() == 3);
	}
}
