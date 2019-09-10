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

	struct tester
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
		auto s1 = sizeof(delegate<void()>);
		auto s2 = sizeof(delegate<int(int, int)>);
		CAGE_TEST(s1 == s2);
		CAGE_LOG(severityEnum::Info, "test", string() + "sizeof(delegate): " + s1);
	}

	{
		CAGE_TESTCASE("free functions");
		delegate<void()> d1 = delegate<void()>().bind<&test1>();
		d1();
		delegate<void(int, int)> d2 = delegate<void(int, int)>().bind<&test2>();
		d2(4, 5);
		delegate<int(int)> d3 = delegate<int(int)>().bind<&test3>();
		d3(6);
	}

	{
		CAGE_TESTCASE("free function overloads");
		delegate<int(uint8)> d4 = delegate<int(uint8)>().bind<&test4>();
		CAGE_TEST(d4(5) == 8);
		delegate<int(uint16)> d5 = delegate<int(uint16)>().bind<&test4>();
		CAGE_TEST(d5(5) == 16);
		delegate<int(uint32)> d6 = delegate<int(uint32)>().bind<&test4>();
		CAGE_TEST(d6(5) == 32);
	}

	{
		CAGE_TESTCASE("free functions with data");
		delegate<int(int, int)> d7 = delegate<int(int, int)>().bind<int, &test5>(13);
		CAGE_TEST(d7(42, 45) == 100);
		delegate<void()> d8 = delegate<void()>().bind<void*, &test6>(nullptr);
		d8();
	}

	{
		CAGE_TESTCASE("methods");
		tester instance;
		delegate<void()> d11 = delegate<void()>().bind<tester, &tester::test11>(&instance);
		d11();
		delegate<void(int, int)> d12 = delegate<void(int, int)>().bind<tester, &tester::test12>(&instance);
		d12(4, 5);
		delegate<int(int)> d13 = delegate<int(int)>().bind<tester, &tester::test13>(&instance);
		d13(6);
	}

	{
		CAGE_TESTCASE("method overloads");
		tester instance;
		delegate<int(uint8)> d14 = delegate<int(uint8)>().bind<tester, &tester::test14>(&instance);
		CAGE_TEST(d14(5) == 8);
		delegate<int(uint16)> d15 = delegate<int(uint16)>().bind<tester, &tester::test14>(&instance);
		CAGE_TEST(d15(5) == 16);
		delegate<int(uint32)> d16 = delegate<int(uint32)>().bind<tester, &tester::test14>(&instance);
		CAGE_TEST(d16(5) == 32);
	}

	{
		CAGE_TESTCASE("const methods");
		const tester instance;
		delegate<void()> d21 = delegate<void()>().bind<tester, &tester::test21>(&instance);
		d21();
		delegate<void(int, int)> d22 = delegate<void(int, int)>().bind<tester, &tester::test22>(&instance);
		d22(4, 5);
		delegate<int(int)> d23 = delegate<int(int)>().bind<tester, &tester::test23>(&instance);
		d23(6);
	}

	{
		CAGE_TESTCASE("const method overloads");
		const tester instance;
		delegate<int(uint8)> d24 = delegate<int(uint8)>().bind<tester, &tester::test24>(&instance);
		CAGE_TEST(d24(5) == 8);
		delegate<int(uint16)> d25 = delegate<int(uint16)>().bind<tester, &tester::test24>(&instance);
		CAGE_TEST(d25(5) == 16);
		delegate<int(uint32)> d26 = delegate<int(uint32)>().bind<tester, &tester::test24>(&instance);
		CAGE_TEST(d26(5) == 32);
	}

	{
		CAGE_TESTCASE("const vs non-const methods");
		tester vi;
		delegate<int()> d27 = delegate<int()>().bind<tester, &tester::test25>(&vi);
		CAGE_TEST(d27() == 0);
		const tester ci;
		delegate<int()> d28 = delegate<int()>().bind<tester, &tester::test25>(&ci);
		CAGE_TEST(d28() == 1);
	}

	{
		CAGE_TESTCASE("static methods");
		delegate<int(int)> d41 = delegate<int(int)>().bind<&tester::test31>();
		d41(7);
	}

	{
		CAGE_TESTCASE("static method overloads");
		delegate<int(uint8)> d44 = delegate<int(uint8)>().bind<&tester::test32>();
		CAGE_TEST(d44(5) == 8);
		delegate<int(uint16)> d45 = delegate<int(uint16)>().bind<&tester::test32>();
		CAGE_TEST(d45(5) == 16);
		delegate<int(uint32)> d46 = delegate<int(uint32)>().bind<&tester::test32>();
		CAGE_TEST(d46(5) == 32);
	}

	{
		CAGE_TESTCASE("static methods with data");
		delegate<int()> d47 = delegate<int()>().bind<int, &tester::test31>(42);
		d47();
	}
}

