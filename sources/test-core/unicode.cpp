#include "main.h"

#include <cage-core/unicode.h>

namespace
{
	void testUtf32()
	{
		const String a = "hello there Straße";
		const auto b = utf8to32(a);
		const String c = utf32to8string(b);
		CAGE_TEST(a == c);
		CAGE_TEST(utf32Length(a) == b->size());
		CAGE_TEST(utf8Length(b) == c.size());
	}
}

void testUnicode()
{
	CAGE_TESTCASE("unicode");
	testUtf32();
	// todo
}
