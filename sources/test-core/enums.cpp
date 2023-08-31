#include "main.h"

#include <cage-core/enumBits.h>

namespace
{
	enum class TestEnum
	{
		TestEnum1,
		TestEnum2,
		TestEnum3,
	};

	enum class TestFlags
	{
		TestFlags0 = 0,
		TestFlags1 = 1 << 0,
		TestFlags2 = 1 << 1,
		TestFlags3 = 1 << 2,
	};
}

namespace cage
{
	GCHL_ENUM_BITS(TestFlags);
}

void testEnums()
{
	CAGE_TESTCASE("enums");

	{
		CAGE_TESTCASE("regular enum class");
		TestEnum a = TestEnum::TestEnum1;
		a = TestEnum::TestEnum2;
		a = (TestEnum)42;
	}

	{
		CAGE_TESTCASE("flags enum class");
		TestFlags a = TestFlags::TestFlags1;
		a = TestFlags::TestFlags2;
		a = (TestFlags)42;

		TestFlags b = TestFlags::TestFlags1 | TestFlags::TestFlags3;
		b |= TestFlags::TestFlags2;
	}

	{
		CAGE_TESTCASE("flags functions any and none");
		CAGE_TEST(any(TestFlags::TestFlags1 | TestFlags::TestFlags3));
		CAGE_TEST(!any(TestFlags::TestFlags0));
		CAGE_TEST(!none(TestFlags::TestFlags1 | TestFlags::TestFlags3));
		CAGE_TEST(none(TestFlags::TestFlags0));
	}
}
