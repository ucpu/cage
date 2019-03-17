#include <vector>

#include "main.h"

namespace
{
	int gCount = 0;

	struct tester
	{
		tester()
		{
			gCount++;
		}

		virtual ~tester()
		{
			gCount--;
		}

		void test(const int expected)
		{
			CAGE_TEST(gCount == expected, gCount, expected);
		}
	};

	void takeByReference(holder<tester> &ts)
	{
		ts->test(1);
	}

	void takeByValue(holder<tester> ts)
	{
		ts->test(1);
	}

	void functionTakingHolder(holder<void> hld)
	{
		CAGE_TEST(gCount == 1);
		hld.clear();
		CAGE_TEST(gCount == 0);
	}
}

void testHolder()
{
	CAGE_TESTCASE("holder");

	{
		CAGE_TESTCASE("takeByReference & takeByValue");
		CAGE_TEST(gCount == 0);
		holder<tester> ts = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 1);
		takeByReference(ts);
		CAGE_TEST(gCount == 1);
		takeByValue(templates::move(ts));
		CAGE_TEST(gCount == 0);
	}
	{
		CAGE_TESTCASE("default ctor & copy ctor & holder<void>");
		CAGE_TEST(gCount == 0);
		holder<tester> a = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 1);
		holder<void> b = a.cast<void>();
		CAGE_TEST(gCount == 1);
		holder<void> c(templates::move(b));
		CAGE_TEST(gCount == 1);
		holder<tester> d = c.cast<tester>();
		CAGE_TEST(gCount == 1);
		d.clear();
		CAGE_TEST(gCount == 0);
	}
	{
		CAGE_TESTCASE("function taking holder");
		CAGE_TEST(gCount == 0);
		holder<tester> a = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 1);
		holder<void> b = a.cast<void>();
		CAGE_TEST(gCount == 1);
		functionTakingHolder(templates::move(b));
		CAGE_TEST(gCount == 0);
	}
	{
		CAGE_TESTCASE("several transfers");
		CAGE_TEST(gCount == 0);
		holder<tester> a = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 1);
		holder<void> b = a.cast<void>();
		CAGE_TEST(gCount == 1);
		holder<tester> c = b.cast<tester>();
		CAGE_TEST(gCount == 1);
		holder<void> d = c.cast<void>();
		CAGE_TEST(gCount == 1);
		holder<tester> e = d.cast<tester>();
		CAGE_TEST(gCount == 1);
		e.clear();
		CAGE_TEST(gCount == 0);
	}
	{
		CAGE_TESTCASE("bool tests");
		CAGE_TEST(gCount == 0);
		holder<tester> a = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 1);
		bool b = (bool)a;
		bool c = !a;
		CAGE_TEST(b);
		CAGE_TEST(!c);
		CAGE_TEST(gCount == 1);
		a.clear();
		CAGE_TEST(gCount == 0);
		b = (bool)a;
		c = !a;
		CAGE_TEST(!b);
		CAGE_TEST(c);
	}
	{
		CAGE_TESTCASE("vector of holders");
		std::vector<holder<tester> > vec;
		vec.resize(2);
		CAGE_TEST(gCount == 0);
		for (uint32 i = 0; i < 2; i++)
			vec[i] = detail::systemArena().createHolder<tester>();
		tester *firstTester = vec[0].get();
		CAGE_TEST(gCount == 2);
		vec.resize(20);
		CAGE_TEST(gCount == 2);
		for (uint32 i = 2; i < 20; i++)
			vec[i] = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 20);
		vec.resize(5);
		CAGE_TEST(gCount == 5);
		vec.resize(100);
		CAGE_TEST(gCount == 5);
		for (uint32 i = 5; i < 100; i++)
			vec[i] = detail::systemArena().createHolder<tester>();
		CAGE_TEST(gCount == 100);
		CAGE_TEST(vec[0].get() == firstTester);
		vec.clear();
		CAGE_TEST(gCount == 0);
	}
}
