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
	};
}

void testDelegates()
{
	CAGE_TESTCASE("delegates");

	{
		CAGE_TESTCASE("sizes");
		auto s1 = sizeof(Delegate<void()>);
		auto s2 = sizeof(Delegate<int(int, int)>);
		CAGE_TEST(s1 == s2);
		CAGE_LOG(SeverityEnum::Info, "test", stringizer() + "sizeof(delegate): " + s1);
	}

	{
		CAGE_TESTCASE("free functions");
		Delegate<void()> d1 = Delegate<void()>().bind<&test1>();
		d1();
		Delegate<void(int, int)> d2 = Delegate<void(int, int)>().bind<&test2>();
		d2(4, 5);
		Delegate<int(int)> d3 = Delegate<int(int)>().bind<&test3>();
		d3(6);
	}

	{
		CAGE_TESTCASE("free function overloads");
		Delegate<int(uint8)> d4 = Delegate<int(uint8)>().bind<&test4>();
		CAGE_TEST(d4(5) == 8);
		Delegate<int(uint16)> d5 = Delegate<int(uint16)>().bind<&test4>();
		CAGE_TEST(d5(5) == 16);
		Delegate<int(uint32)> d6 = Delegate<int(uint32)>().bind<&test4>();
		CAGE_TEST(d6(5) == 32);
	}

	{
		CAGE_TESTCASE("free functions with data");
		Delegate<int(int, int)> d7 = Delegate<int(int, int)>().bind<int, &test5>(13);
		CAGE_TEST(d7(42, 45) == 100);
		Delegate<void()> d8 = Delegate<void()>().bind<void*, &test6>(nullptr);
		d8();
	}

	{
		CAGE_TESTCASE("methods");
		Tester instance;
		Delegate<void()> d11 = Delegate<void()>().bind<Tester, &Tester::test11>(&instance);
		d11();
		Delegate<void(int, int)> d12 = Delegate<void(int, int)>().bind<Tester, &Tester::test12>(&instance);
		d12(4, 5);
		Delegate<int(int)> d13 = Delegate<int(int)>().bind<Tester, &Tester::test13>(&instance);
		d13(6);
	}

	{
		CAGE_TESTCASE("method overloads");
		Tester instance;
		Delegate<int(uint8)> d14 = Delegate<int(uint8)>().bind<Tester, &Tester::test14>(&instance);
		CAGE_TEST(d14(5) == 8);
		Delegate<int(uint16)> d15 = Delegate<int(uint16)>().bind<Tester, &Tester::test14>(&instance);
		CAGE_TEST(d15(5) == 16);
		Delegate<int(uint32)> d16 = Delegate<int(uint32)>().bind<Tester, &Tester::test14>(&instance);
		CAGE_TEST(d16(5) == 32);
	}

	{
		CAGE_TESTCASE("const methods");
		const Tester instance;
		Delegate<void()> d21 = Delegate<void()>().bind<Tester, &Tester::test21>(&instance);
		d21();
		Delegate<void(int, int)> d22 = Delegate<void(int, int)>().bind<Tester, &Tester::test22>(&instance);
		d22(4, 5);
		Delegate<int(int)> d23 = Delegate<int(int)>().bind<Tester, &Tester::test23>(&instance);
		d23(6);
	}

	{
		CAGE_TESTCASE("const method overloads");
		const Tester instance;
		Delegate<int(uint8)> d24 = Delegate<int(uint8)>().bind<Tester, &Tester::test24>(&instance);
		CAGE_TEST(d24(5) == 8);
		Delegate<int(uint16)> d25 = Delegate<int(uint16)>().bind<Tester, &Tester::test24>(&instance);
		CAGE_TEST(d25(5) == 16);
		Delegate<int(uint32)> d26 = Delegate<int(uint32)>().bind<Tester, &Tester::test24>(&instance);
		CAGE_TEST(d26(5) == 32);
	}

	{
		CAGE_TESTCASE("const vs non-const methods");
		Tester vi;
		Delegate<int()> d27 = Delegate<int()>().bind<Tester, &Tester::test25>(&vi);
		CAGE_TEST(d27() == 0);
		const Tester ci;
		Delegate<int()> d28 = Delegate<int()>().bind<Tester, &Tester::test25>(&ci);
		CAGE_TEST(d28() == 1);
	}

	{
		CAGE_TESTCASE("static methods");
		Delegate<int(int)> d41 = Delegate<int(int)>().bind<&Tester::test31>();
		d41(7);
	}

	{
		CAGE_TESTCASE("static method overloads");
		Delegate<int(uint8)> d44 = Delegate<int(uint8)>().bind<&Tester::test32>();
		CAGE_TEST(d44(5) == 8);
		Delegate<int(uint16)> d45 = Delegate<int(uint16)>().bind<&Tester::test32>();
		CAGE_TEST(d45(5) == 16);
		Delegate<int(uint32)> d46 = Delegate<int(uint32)>().bind<&Tester::test32>();
		CAGE_TEST(d46(5) == 32);
	}

	{
		CAGE_TESTCASE("static methods with data");
		Delegate<int()> d47 = Delegate<int()>().bind<int, &Tester::test31>(42);
		d47();
	}
}

