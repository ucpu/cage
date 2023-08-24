#include <vector>

#include "main.h"

namespace
{
	int gCount = 0;
	int dCount = 0;

	struct HolderTester : private Immovable
	{
		HolderTester() { gCount++; }

		virtual ~HolderTester() { gCount--; }
	};

	struct HolderDerived : public HolderTester
	{
		HolderDerived() { dCount++; }

		virtual ~HolderDerived() { dCount--; }
	};

	void takeByReference(Holder<HolderTester> &ts)
	{
		CAGE_TEST(gCount == 1);
	}

	void takeByValue(Holder<HolderTester> ts)
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

	struct IntWrapper : public HolderTester
	{
		int value = 0;
	};

	Holder<IntWrapper> increaseValue(Holder<IntWrapper> &&src)
	{
		Holder<IntWrapper> dst = systemMemory().createHolder<IntWrapper>();
		dst->value = src->value + 10;
		src->value = 0;
		return dst;
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
		Holder<HolderTester> ts = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 1);
		takeByReference(ts);
		CAGE_TEST(gCount == 1);
		takeByValue(std::move(ts));
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("default ctor & copy ctor & holder<void>");
		CAGE_TEST(gCount == 0);
		Holder<HolderTester> a = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = std::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<void> c(std::move(b));
		CAGE_TEST(gCount == 1);
		Holder<HolderTester> d = std::move(c).cast<HolderTester>();
		CAGE_TEST(gCount == 1);
		d.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("dynamic cast failure");
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
		Holder<HolderTester> a = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(a);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 0);
		CAGE_TEST_THROWN(std::move(a).dynCast<HolderDerived>());
		CAGE_TEST(a);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 0);
		a.clear();
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
	}

	{
		CAGE_TESTCASE("downcast with static cast");
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
		Holder<HolderDerived> a = systemMemory().createHolder<HolderDerived>();
		CAGE_TEST(a);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		Holder<HolderTester> b = std::move(a).cast<HolderTester>();
		CAGE_TEST(!a);
		CAGE_TEST(b);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		b.clear();
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
	}

	{
		CAGE_TESTCASE("downcast with dynamic cast");
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
		Holder<HolderDerived> a = systemMemory().createHolder<HolderDerived>();
		CAGE_TEST(a);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		Holder<HolderTester> b = std::move(a).dynCast<HolderTester>();
		CAGE_TEST(!a);
		CAGE_TEST(b);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		b.clear();
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
	}

	{
		CAGE_TESTCASE("dynamic cast (back and forth)");
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
		Holder<HolderDerived> a = systemMemory().createHolder<HolderDerived>();
		CAGE_TEST(a);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		Holder<HolderTester> b = std::move(a).cast<HolderTester>();
		CAGE_TEST(!a);
		CAGE_TEST(b);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		Holder<HolderDerived> c = std::move(b).dynCast<HolderDerived>();
		CAGE_TEST(!b);
		CAGE_TEST(c);
		CAGE_TEST(gCount == 1);
		CAGE_TEST(dCount == 1);
		c.clear();
		CAGE_TEST(gCount == 0);
		CAGE_TEST(dCount == 0);
	}

	{
		CAGE_TESTCASE("function taking holder");
		CAGE_TEST(gCount == 0);
		Holder<HolderTester> a = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = std::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		functionTakingHolder(std::move(b));
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("several transfers");
		CAGE_TEST(gCount == 0);
		Holder<HolderTester> a = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 1);
		Holder<void> b = std::move(a).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<HolderTester> c = std::move(b).cast<HolderTester>();
		CAGE_TEST(gCount == 1);
		Holder<void> d = std::move(c).cast<void>();
		CAGE_TEST(gCount == 1);
		Holder<HolderTester> e = std::move(d).cast<HolderTester>();
		CAGE_TEST(gCount == 1);
		e.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("bool tests");
		CAGE_TEST(gCount == 0);
		Holder<HolderTester> a = systemMemory().createHolder<HolderTester>();
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
		std::vector<Holder<HolderTester>> vec;
		vec.resize(2);
		CAGE_TEST(gCount == 0);
		for (uint32 i = 0; i < 2; i++)
			vec[i] = systemMemory().createHolder<HolderTester>();
		HolderTester *firstTester = vec[0].get();
		CAGE_TEST(gCount == 2);
		vec.resize(20);
		CAGE_TEST(gCount == 2);
		for (uint32 i = 2; i < 20; i++)
			vec[i] = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 20);
		vec.resize(5);
		CAGE_TEST(gCount == 5);
		vec.resize(100);
		CAGE_TEST(gCount == 5);
		for (uint32 i = 5; i < 100; i++)
			vec[i] = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 100);
		CAGE_TEST(vec[0].get() == firstTester);
		std::vector<Holder<HolderTester>> Vec2 = std::move(vec);
		CAGE_TEST(gCount == 100);
		CAGE_TEST(vec.empty());
		Vec2.clear();
		CAGE_TEST(gCount == 0);
	}

	{
		CAGE_TESTCASE("shared holder");
		CAGE_TEST(gCount == 0);
		Holder<HolderTester> a = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 1);
		Holder<HolderTester> b = std::move(a).share();
		CAGE_TEST(gCount == 1);
		CAGE_TEST(a);
		CAGE_TEST(b);
		Holder<HolderTester> c = b.share();
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
		auto s = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(s.get());
		CAGE_TEST(+s);
		HolderTester *raw = +s;
	}

	{
		CAGE_TESTCASE("self assignment");
		auto s = systemMemory().createHolder<HolderTester>();
		CAGE_TEST(gCount == 1);
#if defined(__GNUC__) || defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wself-move"
#endif
		s = std::move(s);
#if defined(__GNUC__) || defined(__clang__)
	#pragma GCC diagnostic pop
#endif
		CAGE_TEST(gCount == 1);
	}

	{
		CAGE_TESTCASE("circular self assignment");
		struct S : public HolderTester
		{
			Holder<S> s;
		};
		Holder<S> a = systemMemory().createHolder<S>();
		a->s = systemMemory().createHolder<S>();
		a->s->s = systemMemory().createHolder<S>();
		a->s->s->s = systemMemory().createHolder<S>();
		CAGE_TEST(gCount == 4);
		a = std::move(a->s);
		CAGE_TEST(gCount == 3);
	}

	{
		CAGE_TESTCASE("simultaneous move from and assign to");
		Holder<IntWrapper> a = systemMemory().createHolder<IntWrapper>();
		a->value = 13;
		a = increaseValue(std::move(a));
		CAGE_TEST(a->value == 23);
	}

	CAGE_TEST(gCount == 0);
	CAGE_TEST(dCount == 0);
}
