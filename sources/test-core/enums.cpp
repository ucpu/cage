#include "main.h"

namespace
{
	enum class testEnum
	{
		testEnum1,
		testEnum2,
		testEnum3,
	};

	enum class testFlags
	{
		testFlags0 = 0,
		testFlags1 = 1 << 0,
		testFlags2 = 1 << 1,
		testFlags3 = 1 << 2,
	};
}

namespace cage
{
	GCHL_ENUM_BITS(testFlags);
}

void testEnums()
{
	CAGE_TESTCASE("enums");

	{
		CAGE_TESTCASE("regular enum class");
		testEnum a = testEnum::testEnum1;
		a = testEnum::testEnum2;
		a = (testEnum)42;
	}

	{
		CAGE_TESTCASE("flags enum class");
		testFlags a = testFlags::testFlags1;
		a = testFlags::testFlags2;
		a = (testFlags)42;

		testFlags b = testFlags::testFlags1 | testFlags::testFlags3;
		b |= testFlags::testFlags2;
	}

	{
		CAGE_TESTCASE("flags functions any and none");
		CAGE_TEST(any(testFlags::testFlags1 | testFlags::testFlags3));
		CAGE_TEST(!any(testFlags::testFlags0));
		CAGE_TEST(!none(testFlags::testFlags1 | testFlags::testFlags3));
		CAGE_TEST(none(testFlags::testFlags0));
	}
}
