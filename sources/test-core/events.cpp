#include "main.h"

#include <cage-core/events.h>

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

	void incrementCallback()
	{
		n += 1;
	}

	void doubleCallback()
	{
		n *= 2;
	}
}

void testEvents()
{
	CAGE_TESTCASE("events");

	{
		CAGE_TESTCASE("basic events");
		n = 0;
		EventDispatcher<bool()> d;
		EventListener<bool()> l1, l2;
		l1.bind<&simpleCallback>();
		l2.bind<&simpleCallback>();
		d.dispatch();
		CAGE_TEST(n == 0);
		d.attach(l1);
		d.attach(l2);
		d.dispatch();
		CAGE_TEST(n == 1);
		l1.detach();
		l1.attach(d);
	}

	{
		CAGE_TESTCASE("void callbacks");
		n = 0;
		EventDispatcher<bool()> d;
		EventListener<void()> l;
		l.bind<&incrementCallback>();
		d.dispatch();
		CAGE_TEST(n == 0);
		d.attach(l);
		d.dispatch();
		CAGE_TEST(n == 1);
		l.detach();
	}

	{
		CAGE_TESTCASE("events with two arguments");
		n = 0;
		EventDispatcher<bool(int, int)> d;
		EventListener<bool(int, int)> l1, l2;
		l1.bind<&simpleCallback>();
		l2.bind<&simpleCallback>();
		d.dispatch(5, 3);
		CAGE_TEST(n == 0);
		d.attach(l1);
		d.attach(l2);
		d.dispatch(5, 3);
		CAGE_TEST(n == 8);
		l1.detach();
		l1.attach(d);
	}

	{
		CAGE_TESTCASE("sorting callbacks");
		n = 3;
		EventDispatcher<bool()> d;
		EventListener<void()> inc, dbl;
		inc.bind<&incrementCallback>();
		dbl.bind<&doubleCallback>();
		d.dispatch();
		CAGE_TEST(n == 3);
		d.attach(inc);
		d.attach(dbl);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
		d.dispatch();
		CAGE_TEST(n == 3);
		d.attach(inc, -5);
		d.attach(dbl, 5);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
		d.attach(inc, 5);
		d.attach(dbl, -5);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 7);
		n = 3;
		d.attach(inc, 5);
		d.attach(dbl, 3);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 7);
		n = 3;
		d.attach(inc, -5);
		d.attach(dbl, -3);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
		d.attach(inc, 0);
		d.attach(dbl, -3);
		d.attach(dbl, 3); // reattaching the same listener should first detach and than attach
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
	}

	{
		CAGE_TESTCASE("attach order");
		{
			n = 0;
			EventDispatcher<bool()> d;
			EventListener<void()> l;
			l.bind<&incrementCallback>();
			CAGE_TEST(n == 0);
			d.attach(l);
			d.dispatch();
			CAGE_TEST(n == 1);
		}
		{
			n = 0;
			EventDispatcher<bool()> d;
			EventListener<void()> l;
			l.bind<&incrementCallback>();
			CAGE_TEST(n == 0);
			l.attach(d);
			d.dispatch();
			CAGE_TEST(n == 1);
		}
	}

	{
		CAGE_TESTCASE("event names");
		EventDispatcher<bool()> d;
		EventListener<bool()> l1, l2;
		d.attach(l1, 13);
		d.attach(l2, 42);
		d.logAllNames();
	}
}
