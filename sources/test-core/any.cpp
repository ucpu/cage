#include "main.h"

#include <cage-core/any.h>

using Any = detail::AnyBase<1024>;

void testAny()
{
	CAGE_TESTCASE("any");

	{
		CAGE_TESTCASE("string literal");
		Any a = StringLiteral("hello world");
		CAGE_TEST(a);
		CAGE_TEST(a.typeHash() == detail::typeHash<StringLiteral>());
		CAGE_TEST(String(a.get<StringLiteral>()) == "hello world");
		a.clear();
		CAGE_TEST(!a);
		CAGE_TEST(a.typeHash() == m);
	}

	{
		CAGE_TESTCASE("string");
		Any a = String("hello world");
		CAGE_TEST(a);
		CAGE_TEST(a.typeHash() == detail::typeHash<String>());
		CAGE_TEST(a.get<String>() == "hello world");
		a.clear();
		CAGE_TEST(!a);
		CAGE_TEST(a.typeHash() == m);
	}

	{
		CAGE_TESTCASE("operator =");
		Any a = String("hello world");
		CAGE_TEST(a);
		CAGE_TEST(a.typeHash() == detail::typeHash<String>());
		CAGE_TEST(a.get<String>() == "hello world");
		a = StringLiteral("abc");
		CAGE_TEST(a);
		CAGE_TEST(a.typeHash() == detail::typeHash<StringLiteral>());
		CAGE_TEST(String(a.get<StringLiteral>()) == "abc");
	}
}
