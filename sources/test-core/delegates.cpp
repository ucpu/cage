#include "main.h"

namespace
{
	void test1() {}
	void test2(int, int) {}
	int test3(int) { return 0; }

	int test4(uint8) { return 8; }
	int test4(uint16) { return 16; }
	int test4(uint32) { return 32; }

	int test5(int a, int b, int c) { return a + b + c; };
	void test6(void *) {};

	void test7() noexcept
	{}

	struct Tester
	{
		void test11() {}
		void test12(int, int) {}
		int test13(int) { return 0; }

		int test14(uint8) { return 8; }
		int test14(uint16) { return 16; }
		int test14(uint32) { return 32; }

		void test21() const {}
		void test22(int, int) const {}
		int test23(int) const { return 0; }

		int test24(uint8) const { return 8; }
		int test24(uint16) const { return 16; }
		int test24(uint32) const { return 32; }

		int test25() { return 0; }
		int test25() const { return 1; }

		static int test31(int) { return 0; }

		static int test32(uint8) { return 8; }
		static int test32(uint16) { return 16; }
		static int test32(uint32) { return 32; }

		void test41() const {}
		void test42() noexcept {}
		void test43() const noexcept {}
	};

	constexpr int constexprSum(int a, int b)
	{
		return a + b;
	}

	constexpr int constexprTest()
	{
		constexpr auto del = Delegate<int(int, int)>().bind<&constexprSum>();
		return del(2, 3);
	}
}

void testDelegates()
{
	CAGE_TESTCASE("delegates");

	{
		CAGE_TESTCASE("sizes");
		constexpr auto s1 = sizeof(Delegate<void()>);
		constexpr auto s2 = sizeof(Delegate<int(int, int)>);
		CAGE_TEST(s1 == s2);
		CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "sizeof(delegate): " + s1);
	}

	{
		CAGE_TESTCASE("free functions");
		Delegate d1 = Delegate<void()>().bind<&test1>();
		d1();
		Delegate d2 = Delegate<void(int, int)>().bind<&test2>();
		d2(4, 5);
		Delegate d3 = Delegate<int(int)>().bind<&test3>();
		d3(6);
	}

	{
		CAGE_TESTCASE("free function overloads");
		Delegate d4 = Delegate<int(uint8)>().bind<&test4>();
		CAGE_TEST(d4(5) == 8);
		Delegate d5 = Delegate<int(uint16)>().bind<&test4>();
		CAGE_TEST(d5(5) == 16);
		Delegate d6 = Delegate<int(uint32)>().bind<&test4>();
		CAGE_TEST(d6(5) == 32);
	}

	{
		CAGE_TESTCASE("free functions with data");
		Delegate d7 = Delegate<int(int, int)>().bind<int, &test5>(13);
		CAGE_TEST(d7(42, 45) == 100);
		Delegate d8 = Delegate<void()>().bind<void*, &test6>(nullptr);
		d8();
	}

	{
		CAGE_TESTCASE("methods");
		Tester instance;
		Delegate d11 = Delegate<void()>().bind<Tester, &Tester::test11>(&instance);
		d11();
		Delegate d12 = Delegate<void(int, int)>().bind<Tester, &Tester::test12>(&instance);
		d12(4, 5);
		Delegate d13 = Delegate<int(int)>().bind<Tester, &Tester::test13>(&instance);
		d13(6);
	}

	{
		CAGE_TESTCASE("methods overloads");
		Tester instance;
		Delegate d14 = Delegate<int(uint8)>().bind<Tester, &Tester::test14>(&instance);
		CAGE_TEST(d14(5) == 8);
		Delegate d15 = Delegate<int(uint16)>().bind<Tester, &Tester::test14>(&instance);
		CAGE_TEST(d15(5) == 16);
		Delegate d16 = Delegate<int(uint32)>().bind<Tester, &Tester::test14>(&instance);
		CAGE_TEST(d16(5) == 32);
	}

	{
		CAGE_TESTCASE("const methods");
		const Tester instance;
		Delegate d21 = Delegate<void()>().bind<Tester, &Tester::test21>(&instance);
		d21();
		Delegate d22 = Delegate<void(int, int)>().bind<Tester, &Tester::test22>(&instance);
		d22(4, 5);
		Delegate d23 = Delegate<int(int)>().bind<Tester, &Tester::test23>(&instance);
		d23(6);
	}

	{
		CAGE_TESTCASE("const methods overloads");
		const Tester instance;
		Delegate d24 = Delegate<int(uint8)>().bind<Tester, &Tester::test24>(&instance);
		CAGE_TEST(d24(5) == 8);
		Delegate d25 = Delegate<int(uint16)>().bind<Tester, &Tester::test24>(&instance);
		CAGE_TEST(d25(5) == 16);
		Delegate d26 = Delegate<int(uint32)>().bind<Tester, &Tester::test24>(&instance);
		CAGE_TEST(d26(5) == 32);
	}

	{
		CAGE_TESTCASE("const vs non-const methods");
		Tester vi;
		Delegate d27 = Delegate<int()>().bind<Tester, &Tester::test25>(&vi);
		CAGE_TEST(d27() == 0);
		const Tester ci;
		Delegate d28 = Delegate<int()>().bind<Tester, &Tester::test25>(&ci);
		CAGE_TEST(d28() == 1);
	}

	{
		CAGE_TESTCASE("static methods");
		Delegate d41 = Delegate<int(int)>().bind<&Tester::test31>();
		d41(7);
	}

	{
		CAGE_TESTCASE("static methods overloads");
		Delegate d44 = Delegate<int(uint8)>().bind<&Tester::test32>();
		CAGE_TEST(d44(5) == 8);
		Delegate d45 = Delegate<int(uint16)>().bind<&Tester::test32>();
		CAGE_TEST(d45(5) == 16);
		Delegate d46 = Delegate<int(uint32)>().bind<&Tester::test32>();
		CAGE_TEST(d46(5) == 32);
	}

	{
		CAGE_TESTCASE("static methods with data");
		Delegate d47 = Delegate<int()>().bind<int, &Tester::test31>(42);
		d47();
	}

	{
		CAGE_TESTCASE("noexcept functions and methods");
		Tester vi;
		Delegate d0 = Delegate<void()>().bind<test7>();
		d0();
		Delegate d1 = Delegate<void()>().bind<Tester, &Tester::test41>(&vi);
		d1();
		Delegate d2 = Delegate<void()>().bind<Tester, &Tester::test42>(&vi);
		d2();
		Delegate d3 = Delegate<void()>().bind<Tester, &Tester::test43>(&vi);
		d3();
	}

	{
		CAGE_TESTCASE("constexpr delegate");
		constexpr auto val = constexprTest();
		static_assert(val == 5);
	}

	{
		CAGE_TESTCASE("delegate with lambda (simple)");
		Delegate d = Delegate<int(int, int)>().bind([](int a, int b) { return a + b; });
		CAGE_TEST(d(42, 13) == 42 + 13);
	}

	{
		CAGE_TESTCASE("delegate with lambda (with capture)");
		int tmp = 500;
		Delegate d = Delegate<void(int)>().bind([&](int a) { tmp += a; });
		d(42);
		CAGE_TEST(tmp == 542);
	}

	{
		CAGE_TESTCASE("delegate with lambda (in constructor)");
		int tmp = 500;
		Delegate d = Delegate<void(int)>([&](int a) { tmp += a; });
		d(42);
		CAGE_TEST(tmp == 542);
	}

	{
		CAGE_TESTCASE("delegate with lambda (direct assignment)");
		int tmp = 500;
		Delegate<void(int)> d = [&](int a) { tmp += a; };
		d(42);
		CAGE_TEST(tmp == 542);
	}

	{
		CAGE_TESTCASE("function in constructor");
		Delegate<void(int, int)> d = &test2;
		d(3, 4);
	}
}

