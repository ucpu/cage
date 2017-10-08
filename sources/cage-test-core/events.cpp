#include "main.h"

namespace
{
	uint32 n;

	bool simpleCallback()
	{
		n += 1;
		return true;
	}

	bool simpleCallback(int a, int b)
	{
		n += a + b;
		return true;
	}
}

void testEvents()
{
	CAGE_TESTCASE("events");

	{
		CAGE_TESTCASE("basic events");
		n = 0;
		eventDispatcher<bool()> d;
		eventDispatcher<bool()>::listenerType l1, l2;
		l1.bind<&simpleCallback>();
		l2.bind<&simpleCallback>();
		d.dispatch();
		CAGE_TEST(n == 0);
		d.add(l1);
		d.add(l2);
		d.dispatch();
		CAGE_TEST(n == 1);
		l1.detach();
		l1.attach(d);
	}

	{
		CAGE_TESTCASE("copy ctor, op=");
		n = 0;
		eventDispatcher<bool()> d1, d2;
		eventDispatcher<bool()>::listenerType l1, l2;
		l1.bind<&simpleCallback>();
		l2.bind<&simpleCallback>();
		l1.attach(d1);
		d1.dispatch();
		CAGE_TEST(n == 1);
		l1 = l2;
		d1.dispatch();
		CAGE_TEST(n == 1);
	}

	{
		CAGE_TESTCASE("events with two arguments");
		n = 0;
		eventDispatcher<bool(int, int)> d;
		eventDispatcher<bool(int, int)>::listenerType l1, l2;
		l1.bind<&simpleCallback>();
		l2.bind<&simpleCallback>();
		d.dispatch(5, 3);
		CAGE_TEST(n == 0);
		d.add(l1);
		d.add(l2);
		d.dispatch(5, 3);
		CAGE_TEST(n == 8);
		l1.detach();
		l1.attach(d);
	}
}