#include <vector>

#include "main.h"

#include <cage-core/unicode.h>

namespace
{
	void testUtf32()
	{
		const String a = "hello there Straße";
		CAGE_TEST(utfValid(a));
		const auto b = utf8to32(a);
		const String c = utf32to8string(b);
		CAGE_TEST(a == c);
		CAGE_TEST(utf32Length(a) == b->size());
		CAGE_TEST(utf8Length(b) == c.size());
		CAGE_TEST(utfValid(""));
		CAGE_TEST(utf8to32("").empty());

		{
			std::vector<uint32> vec;
			vec.resize(utf32Length(a) + 5);
			PointerRange<uint32> range = vec;
			utf8to32(range, a);
			CAGE_TEST(range.size() == utf32Length(a));
			const String d = utf32to8string(range);
			CAGE_TEST(a == d);
		}

		{
			std::vector<uint32> vec;
			PointerRange<uint32> range = vec;
			utf8to32(range, "");
			CAGE_TEST(range.empty());
		}
	}

	void testValidation()
	{
		CAGE_TEST(utfValid("hello there"));
		CAGE_TEST(!utfValid("Te\xC2st")); // "\xC2" is truncated sequence in UTF-8
		CAGE_TEST(unicodeTransformString("Te\xC2st", { UnicodeTransformEnum::Validate }) == "Te\xEF\xBF\xBDst"); // "\xEF\xBF\xBD" is the replacement character U+FFFD in UTF-8
	}

	void testCaseConversions()
	{
		CAGE_TEST(unicodeTransformString("heLLo There", { UnicodeTransformEnum::Lowercase }) == "hello there");
		CAGE_TEST(unicodeTransformString("heLLo There", { UnicodeTransformEnum::Uppercase }) == "HELLO THERE");
		CAGE_TEST(unicodeTransformString("heLLo There", { UnicodeTransformEnum::Titlecase }) == "Hello There");
		CAGE_TEST(unicodeTransformString("heLLo There", { UnicodeTransformEnum::Casefold }) == "hello there");

		CAGE_TEST(unicodeTransformString("Příliš Žluťoučký kůň úpĚl ďÁbelské ódy", { UnicodeTransformEnum::Lowercase }) == "příliš žluťoučký kůň úpěl ďábelské ódy");
		CAGE_TEST(unicodeTransformString("Příliš Žluťoučký kůň úpĚl ďÁbelské ódy", { UnicodeTransformEnum::Uppercase }) == "PŘÍLIŠ ŽLUŤOUČKÝ KŮŇ ÚPĚL ĎÁBELSKÉ ÓDY");
		CAGE_TEST(unicodeTransformString("Příliš Žluťoučký kůň úpĚl ďÁbelské ódy", { UnicodeTransformEnum::Titlecase }) == "Příliš Žluťoučký Kůň Úpěl Ďábelské Ódy");
		CAGE_TEST(unicodeTransformString("Příliš Žluťoučký kůň úpĚl ďÁbelské ódy", { UnicodeTransformEnum::Casefold }) == "příliš žluťoučký kůň úpěl ďábelské ódy");
		CAGE_TEST(unicodeTransformString("Příliš Žluťoučký kůň úpĚl ďÁbelské ódy", { .transform = UnicodeTransformEnum::Lowercase, .locale = "cs_cz" }) == "příliš žluťoučký kůň úpěl ďábelské ódy");

		CAGE_TEST(unicodeTransformString("ijslAnd", { UnicodeTransformEnum::Titlecase }) == "Ijsland");
		CAGE_TEST(unicodeTransformString("ijslAnd", { .transform = UnicodeTransformEnum::Lowercase, .locale = "nl" }) == "ijsland");
		CAGE_TEST(unicodeTransformString("ijslAnd", { .transform = UnicodeTransformEnum::Uppercase, .locale = "nl" }) == "IJSLAND");
		CAGE_TEST(unicodeTransformString("ijslAnd", { .transform = UnicodeTransformEnum::Titlecase, .locale = "nl" }) == "IJsland");
	}

	void testNormalizations()
	{
		CAGE_TEST(unicodeTransformString("heLLo There", { UnicodeTransformEnum::CanonicalComposition }) == "heLLo There");
		CAGE_TEST(unicodeTransformString("Ŵ W\u0302", { UnicodeTransformEnum::CanonicalComposition }) == "Ŵ Ŵ");
		CAGE_TEST(unicodeTransformString("Ŵ W\u0302", { UnicodeTransformEnum::CanonicalDecomposition }) == "W\u0302 W\u0302");
	}

	void testFuzzyMatching()
	{
		CAGE_TEST(unicodeTransformString("Příliš Žluťoučký kůň úpĚl ďÁbelské ódy. Hello there.", { UnicodeTransformEnum::FuzzyMatching }) == "prilis zlutoucky kun upel dabelske ody. helo there.");
	}
}

void testUnicode()
{
	CAGE_TESTCASE("unicode");
	testUtf32();
	testValidation();
	testCaseConversions();
	testNormalizations();
	testFuzzyMatching();
}
