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

	bool returnBoolCallback(bool retval)
	{
		return retval;
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
		l1.bind<bool (*)()>(&simpleCallback);
		l2.bind<bool (*)()>(&simpleCallback);
		d.dispatch();
		CAGE_TEST(n == 0);
		l1.attach(d);
		l2.attach(d);
		d.dispatch();
		CAGE_TEST(n == 1);
		l1.detach();
		l1.attach(d);
		l1.unbind();
	}

	{
		CAGE_TESTCASE("void callbacks");
		n = 0;
		EventDispatcher<bool()> d;
		EventListener<bool()> l;
		l.bind(&incrementCallback);
		d.dispatch();
		CAGE_TEST(n == 0);
		l.attach(d);
		d.dispatch();
		CAGE_TEST(n == 1);
		l.detach();
	}

	{
		CAGE_TESTCASE("events with two arguments");
		n = 0;
		EventDispatcher<bool(int, int)> d;
		EventListener<bool(int, int)> l1, l2;
		l1.bind<bool (*)(int, int)>(&simpleCallback);
		l2.bind<bool (*)(int, int)>(&simpleCallback);
		d.dispatch(5, 3);
		CAGE_TEST(n == 0);
		l1.attach(d);
		l2.attach(d);
		d.dispatch(5, 3);
		CAGE_TEST(n == 8);
		l1.detach();
		l1.attach(d);
	}

	{
		CAGE_TESTCASE("sorting callbacks");
		n = 3;
		EventDispatcher<bool()> d;
		EventListener<bool()> inc, dbl;
		inc.bind(&incrementCallback);
		dbl.bind(&doubleCallback);
		d.dispatch();
		CAGE_TEST(n == 3);
		inc.attach(d);
		dbl.attach(d);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
		d.dispatch();
		CAGE_TEST(n == 3);
		inc.attach(d, -5);
		dbl.attach(d, 5);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
		inc.attach(d, 5);
		dbl.attach(d, -5);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 7);
		n = 3;
		inc.attach(d, 5);
		dbl.attach(d, 3);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 7);
		n = 3;
		inc.attach(d, -5);
		dbl.attach(d, -3);
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
		inc.attach(d, 0);
		dbl.attach(d, -3);
		dbl.attach(d, 3); // reattaching the same listener should first detach and than attach
		d.dispatch();
		d.detach();
		CAGE_TEST(n == 8);
		n = 3;
	}

	{
		CAGE_TESTCASE("event names");
		EventDispatcher<bool()> d;
		EventListener<bool()> l1, l2;
		l1.attach(d, 13);
		l2.attach(d, 42);
		d.logAllNames();
	}

	{
		CAGE_TESTCASE("return values");
		EventDispatcher<bool(bool)> d;
		EventListener<bool(bool)> l;
		l.attach(d);
		l.bind(&returnBoolCallback);
		CAGE_TEST(d.dispatch(true) == true);
		CAGE_TEST(d.dispatch(false) == false);
	}

	{
		CAGE_TESTCASE("simple listener");
		{
			n = 0;
			EventDispatcher<bool(int, int)> d;
			auto l = d.listen<bool (*)(int, int)>(&simpleCallback);
			CAGE_TEST(n == 0);
			CAGE_TEST(d.dispatch(42, 13) == true);
			CAGE_TEST(n == 42 + 13);
		}
		{
			n = 0;
			EventDispatcher<bool(int, int)> d;
			auto l = d.listen([](int a, int b) { return simpleCallback(a, b); });
			CAGE_TEST(n == 0);
			CAGE_TEST(d.dispatch(42, 13) == true);
			CAGE_TEST(n == 42 + 13);
		}
		{
			EventDispatcher<bool(bool)> d;
			auto l = d.listen(&returnBoolCallback);
			CAGE_TEST(d.dispatch(true) == true);
			CAGE_TEST(d.dispatch(false) == false);
		}
		{
			EventDispatcher<bool()> d;
			auto l = d.listen([]() { return true; });
			CAGE_TEST(d.dispatch() == true);
		}
		{
			EventDispatcher<bool()> d;
			auto l = d.listen([]() -> void {});
			CAGE_TEST(d.dispatch() == false);
		}
	}

	{
		CAGE_TESTCASE("lambda listener with capture");
		int callcount = 0;
		EventDispatcher<bool()> d;
		auto l = d.listen([&]() { callcount++; });
		CAGE_TEST(callcount == 0);
		CAGE_TEST(d.dispatch() == false);
		CAGE_TEST(callcount == 1);
	}

	{
		CAGE_TESTCASE("listener with function pointer only (void)");
		n = 13;
		EventDispatcher<bool()> d;
		auto l = d.listen(&doubleCallback);
		CAGE_TEST(n == 13);
		CAGE_TEST(d.dispatch() == false);
		CAGE_TEST(n == 13 * 2);
	}

	{
		CAGE_TESTCASE("listener with function pointer only (bool)");
		EventDispatcher<bool(bool)> d;
		auto l = d.listen(&returnBoolCallback);
		CAGE_TEST(d.dispatch(true) == true);
		CAGE_TEST(d.dispatch(false) == false);
	}

	{
		CAGE_TESTCASE("listener with class (void)");
		struct Tmp
		{
			int b = 0;
			void call(int a) { b = a; }
		};
		EventDispatcher<bool(int)> d;
		Tmp tmp;
		auto l = d.listen(Delegate<void(int)>().bind<Tmp, &Tmp::call>(&tmp));
		CAGE_TEST(tmp.b == 0);
		CAGE_TEST(d.dispatch(13) == false);
		CAGE_TEST(tmp.b == 13);
	}

	{
		CAGE_TESTCASE("listener with class (bool)");
		struct Tmp
		{
			int b = 0;
			bool call(int a)
			{
				b = a;
				return false;
			}
		};
		EventDispatcher<bool(int)> d;
		Tmp tmp;
		auto l = d.listen(Delegate<bool(int)>().bind<Tmp, &Tmp::call>(&tmp));
		CAGE_TEST(tmp.b == 0);
		CAGE_TEST(d.dispatch(13) == false);
		CAGE_TEST(tmp.b == 13);
	}
}
