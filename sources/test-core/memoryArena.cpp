#include "main.h"

namespace
{
	struct TestBase : private Immovable
	{
		TestBase() { baseCounter++; }

		virtual ~TestBase() { baseCounter--; }

		static inline sint32 baseCounter = 0;
	};

	struct TestDerived : public TestBase
	{
		TestDerived() { derivedCounter++; }

		~TestDerived() { derivedCounter--; }

		static inline sint32 derivedCounter = 0;
	};

	void testWithHolder()
	{
		CAGE_TESTCASE("with holder");
		{
			Holder<TestDerived> arr[10];
			CAGE_TEST(TestDerived::baseCounter == 0);
			CAGE_TEST(TestDerived::derivedCounter == 0);
			for (auto &it : arr)
				it = systemMemory().createHolder<TestDerived>();
			CAGE_TEST(TestDerived::baseCounter == 10);
			CAGE_TEST(TestDerived::derivedCounter == 10);
		}
		CAGE_TEST(TestDerived::baseCounter == 0);
		CAGE_TEST(TestDerived::derivedCounter == 0);
	}

	void testWithExplicitDestroyOnDerived()
	{
		CAGE_TESTCASE("explicit destroy on derived");
		TestDerived *arr[10];
		CAGE_TEST(TestDerived::baseCounter == 0);
		CAGE_TEST(TestDerived::derivedCounter == 0);
		for (auto &it : arr)
			it = systemMemory().createObject<TestDerived>();
		CAGE_TEST(TestDerived::baseCounter == 10);
		CAGE_TEST(TestDerived::derivedCounter == 10);
		for (auto &it : arr)
			systemMemory().destroy<TestDerived>(it);
		CAGE_TEST(TestDerived::baseCounter == 0);
		CAGE_TEST(TestDerived::derivedCounter == 0);
	}

	void testWithExplicitDestroyOnBase()
	{
		CAGE_TESTCASE("explicit destroy on base");
		TestDerived *arr[10];
		CAGE_TEST(TestDerived::baseCounter == 0);
		CAGE_TEST(TestDerived::derivedCounter == 0);
		for (auto &it : arr)
			it = systemMemory().createObject<TestDerived>();
		CAGE_TEST(TestDerived::baseCounter == 10);
		CAGE_TEST(TestDerived::derivedCounter == 10);
		for (auto &it : arr)
			systemMemory().destroy<TestBase>(it);
		CAGE_TEST(TestDerived::baseCounter == 0);
		CAGE_TEST(TestDerived::derivedCounter == 0);
	}
}

void testMemoryArena()
{
	CAGE_TESTCASE("memory arena");

	testWithHolder();
	testWithExplicitDestroyOnDerived();
	testWithExplicitDestroyOnBase();
}
