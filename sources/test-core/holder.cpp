#include "main.h"

#include <vector>

namespace
{
	int gCount = 0;

	struct Tester
	{
		Tester()
		{
			gCount++;
		}

		virtual ~Tester()
		{
			gCount--;
		}

		void test(const int expected)
		{
			CAGE_TEST(gCount == expected, gCount, expected);
		}
	};

	void takeByReference(Holder<Tester> &ts)
	{
		ts->test(1);
	}

	void takeByValue(Holder<Tester> ts)
	{
		ts->test(1);
	}

	void functionTakingHolder(Holder<void> hld)
	{
		CAGE_TEST(gCount == 1);
		hld.clear();
		CAGE_TEST(gCount == 0);
	}

	struct ForwardDeclared; // does not have full declaration

	Holder<ForwardDeclared> functionReturningForwardDeclared()
	{
		return {};
	}
	
	void functionTakingForwardDeclared(Holder<ForwardDeclared> &&param)
	{
		Holder<ForwardDeclared> local = templates::move(param);
		(void)local;
	}
}

void testHolder()
{
	CAGE_TESTCASE("holder");

	{
		CAGE_TESTCASE("forward declared only");
		Holder<ForwardDeclared> h = functionReturningForwardDeclared();
		functionTakingForwardDeclared(templates::move(h));
	}

	{
		CAGE_TESTCASE("takeByReference & takeByValue");
		CAGE_TEST(gCount == 0);
		Holder<Tester> ts = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		takeByReference(ts);
		CAGE_TEST(gCount == 1);
		takeByValue(templates::move(ts));
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("default ctor & copy ctor & holder<void>");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = templates::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<void> c(templates::move(b));
		CAGE_TEST(gCount == 1);
		Holder<Tester> d = templates::move(c).cast<Tester>();
		CAGE_TEST(gCount == 1);
		d.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("function taking holder");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = templates::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		functionTakingHolder(templates::move(b));
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("several transfers");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = templates::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<Tester> c = templates::move(b).cast<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> d = templates::move(c).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<Tester> e = templates::move(d).cast<Tester>();
		CAGE_TEST(gCount == 1);
		e.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("bool tests");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = detail::systemArena().createHolder<Tester>();
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
		std::vector<Holder<Tester>> vec;
		vec.resize(2);
		CAGE_TEST(gCount == 0);
		for (uint32 i = 0; i < 2; i++)
			vec[i] = detail::systemArena().createHolder<Tester>();
		Tester *firstTester = vec[0].get();
		CAGE_TEST(gCount == 2);
		vec.resize(20);
		CAGE_TEST(gCount == 2);
		for (uint32 i = 2; i < 20; i++)
			vec[i] = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 20);
		vec.resize(5);
		CAGE_TEST(gCount == 5);
		vec.resize(100);
		CAGE_TEST(gCount == 5);
		for (uint32 i = 5; i < 100; i++)
			vec[i] = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 100);
		CAGE_TEST(vec[0].get() == firstTester);
		std::vector<Holder<Tester>> vec2 = std::move(vec);
		CAGE_TEST(gCount == 100);
		CAGE_TEST(vec.empty());
		vec2.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("shared holder");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(!a.isShareable());
		Holder<Tester> b = templates::move(a).makeShareable();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(!a);
		CAGE_TEST(b);
		CAGE_TEST(b.isShareable());
		Holder<Tester> c = b.share();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(b);
		CAGE_TEST(c);
		CAGE_TEST(b.get() == c.get());
		CAGE_TEST(b.isShareable());
		CAGE_TEST(c.isShareable());
		b.clear();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(!b);
		CAGE_TEST(c);
		CAGE_TEST(c.isShareable());
		c.clear();
		CAGE_TEST(gCount == 0);
		CAGE_TEST(!c);
	}

	{
		CAGE_TESTCASE("get and operator +");
		auto s = detail::systemArena().createHolder<Tester>();
		CAGE_TEST(s.get());
		CAGE_TEST(+s);
		Tester *raw = +s;
	}
}
