#include "main.h"

#include <vector>

namespace
{
	int gCount = 0;

	struct Tester : Immovable
	{
		Tester()
		{
			gCount++;
		}

		virtual ~Tester()
		{
			gCount--;
		}
	};

	struct Derived : public Tester
	{};

	void takeByReference(Holder<Tester> &ts)
	{
		CAGE_TEST(gCount == 1);
	}

	void takeByValue(Holder<Tester> ts)
	{
		CAGE_TEST(gCount == 1);
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
		Holder<ForwardDeclared> local = std::move(param);
	}
}

void testHolder()
{
	CAGE_TESTCASE("holder");

	{
		CAGE_TESTCASE("forward declared only");
		Holder<ForwardDeclared> h = functionReturningForwardDeclared();
		functionTakingForwardDeclared(std::move(h));
	}

	{
		CAGE_TESTCASE("takeByReference & takeByValue");
		CAGE_TEST(gCount == 0);
		Holder<Tester> ts = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		takeByReference(ts);
		CAGE_TEST(gCount == 1);
		takeByValue(std::move(ts));
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("default ctor & copy ctor & holder<void>");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = std::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<void> c(std::move(b));
		CAGE_TEST(gCount == 1);
		Holder<Tester> d = std::move(c).cast<Tester>();
		CAGE_TEST(gCount == 1);
		d.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("dynamic cast");
		CAGE_TEST(gCount == 0);
		Holder<Derived> a = systemArena().createHolder<Derived>();
		CAGE_TEST(gCount == 1);
		Holder<Tester> b = std::move(a).cast<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<Derived> c = std::move(b).dynCast<Derived>();
		CAGE_TEST(gCount == 1);
		c.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("function taking holder");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = std::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		functionTakingHolder(std::move(b));
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("several transfers");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = std::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<Tester> c = std::move(b).cast<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<void> d = std::move(c).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<Tester> e = std::move(d).cast<Tester>();
		CAGE_TEST(gCount == 1);
		e.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("bool tests");
		CAGE_TEST(gCount == 0);
		Holder<Tester> a = systemArena().createHolder<Tester>();
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
			vec[i] = systemArena().createHolder<Tester>();
		Tester *firstTester = vec[0].get();
		CAGE_TEST(gCount == 2);
		vec.resize(20);
		CAGE_TEST(gCount == 2);
		for (uint32 i = 2; i < 20; i++)
			vec[i] = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 20);
		vec.resize(5);
		CAGE_TEST(gCount == 5);
		vec.resize(100);
		CAGE_TEST(gCount == 5);
		for (uint32 i = 5; i < 100; i++)
			vec[i] = systemArena().createHolder<Tester>();
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
		Holder<Tester> a = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		Holder<Tester> b = std::move(a).share();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(a);
		CAGE_TEST(b);
		Holder<Tester> c = b.share();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(b);
		CAGE_TEST(c);
		CAGE_TEST(+b == +c);
		b.clear();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(!b);
		CAGE_TEST(c);
		a.clear();
		c.clear();
		CAGE_TEST(gCount == 0);
		CAGE_TEST(!c);
	}

	{
		CAGE_TESTCASE("get and operator +");
		auto s = systemArena().createHolder<Tester>();
		CAGE_TEST(s.get());
		CAGE_TEST(+s);
		Tester *raw = +s;
	}

	{
		CAGE_TESTCASE("self assignment");
		auto s = systemArena().createHolder<Tester>();
		CAGE_TEST(gCount == 1);
		s = std::move(s);
		CAGE_TEST(gCount == 1);
	}

	CAGE_TEST(gCount == 0);
}
