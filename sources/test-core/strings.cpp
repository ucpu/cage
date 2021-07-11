#include "main.h"
#include <cage-core/math.h>
#include <cage-core/files.h>
#include <cage-core/hashString.h>

#include <cstring>
#include <cmath> // std::abs
#include <map>

void test(real a, real b);

namespace
{
	void test(float a, float b)
	{
		CAGE_TEST(std::abs(a - b) < 1e-5);
	}

	void test(double a, double b)
	{
		CAGE_TEST(std::abs(a - b) < 1e-12);
	}

	constexpr StringLiteral pickName(uint32 i)
	{
		switch (i)
		{
		case 0: return "zero";
		case 1: return "one";
		case 2: return "two";
		case 3: return "three";
		default: return "too much";
		}
	}

	void testStringLiterals()
	{
		CAGE_TESTCASE("string literals");
		constexpr StringLiteral one = pickName(1);
		CAGE_TEST(string(one) == "one");
	}

	void testBasics()
	{
		{
			CAGE_TESTCASE("constructors and operator +");
			CAGE_TEST(string("ra") + "ke" + "ta" == "raketa");
			bool b = true;
			CAGE_TEST(string(stringizer() + b) == "true");
			b = false;
			CAGE_TEST(string(stringizer() + b) == "false");
			int i1 = -123;
			CAGE_TEST(string(stringizer() + i1) == "-123");
			unsigned int i2 = 123;
			CAGE_TEST(string(stringizer() + i2) == "123");
			sint8 i3 = 123;
			CAGE_TEST(string(stringizer() + i3) == "123");
			sint16 i4 = 123;
			CAGE_TEST(string(stringizer() + i4) == "123");
			sint32 i5 = 123;
			CAGE_TEST(string(stringizer() + i5) == "123");
			sint64 i6 = 123;
			CAGE_TEST(string(stringizer() + i6) == "123");
			uint8 i7 = 123;
			CAGE_TEST(string(stringizer() + i7) == "123");
			uint16 i8 = 123;
			CAGE_TEST(string(stringizer() + i8) == "123");
			uint32 i9 = 123;
			CAGE_TEST(string(stringizer() + i9) == "123");
			uint64 i10 = 123;
			CAGE_TEST(string(stringizer() + i10) == "123");
			{
				float f = 5;
				string fs = stringizer() + f;
				CAGE_TEST(fs == "5" || fs == "5.000000");
				double d = 5;
				string ds = stringizer() + d;
				CAGE_TEST(ds == "5" || ds == "5.000000");
			}
			{
				float f = 5.5;
				string fs = stringizer() + f;
				CAGE_TEST(fs == "5.5" || fs == "5.500000");
				double d = 5.5;
				string ds = stringizer() + d;
				CAGE_TEST(ds == "5.5" || ds == "5.500000");
			}
			const char arr[] = "array";
			CAGE_TEST(string(arr) == "array");
			char arr2[] = "array";
			CAGE_TEST(string(arr2) == "array");
		}
		{
			CAGE_TESTCASE("comparisons == and != and length");
			CAGE_TEST(string("") == string(""));
			string a = "ahoj";
			CAGE_TEST(a == "ahoj");
			string b = "nazdar";
			CAGE_TEST(b == "nazdar");
			CAGE_TEST(a != b);
			CAGE_TEST(a.length() == 4);
			CAGE_TEST(b.length() == 6);
			string c = a + b;
			CAGE_TEST(c.length() == 10);
			CAGE_TEST(c == "ahojnazdar");
		}
		{
			CAGE_TESTCASE("comparisons < and <= and > and >=");
			CAGE_TEST(string("") < string("bbb"));
			CAGE_TEST(string("cedr") > string("bedr"));
			CAGE_TEST(string("cedr") > string("ceda"));
			CAGE_TEST(string("cedr") < string("dedr"));
			CAGE_TEST(string("cedr") < string("cedz"));
			CAGE_TEST(string("cedr") <= string("cedr"));
			CAGE_TEST(string("cedr") >= string("cedr"));
			CAGE_TEST(!(string("cedr") >= string("kapr")));
			CAGE_TEST(string("romeo") > string("rom"));
			CAGE_TEST(string("rom") < string("romeo"));
		}
		{
			CAGE_TESTCASE("operator []");
			const string a = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
			CAGE_TEST(a[0] == 'r');
			CAGE_TEST(a[6] == ':');
			CAGE_TEST(a[a.length() - 1] == 'n');
		}
		{
			CAGE_TESTCASE("c_str");
			CAGE_TEST(string().c_str() != nullptr);
			const string a = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
			const char *tmp = a.c_str();
			CAGE_TEST(strlen(tmp) == a.length());
			for (uint32 i = 0; i < a.length(); i++)
				CAGE_TEST(a[i] == tmp[i]);
		}
		{
			CAGE_TESTCASE("pointer range");
			{
				PointerRange<const char> r = "kokos"; // todo fix does not work with constexpr!
				CAGE_TEST(r.size() == 5); // the terminal zero is automatically removed
				CAGE_TEST(r[0] == 'k');
			}
			{
				PointerRange<const char> r = "kokos"; // todo fix does not work with constexpr!
				const string s = string(r);
				CAGE_TEST(s.size() == 5);
				CAGE_TEST(s[0] == 'k');
			}
			{
				string s = "kokos";
				CAGE_TEST(s.size() == 5);
				CAGE_TEST(s[0] == 'k');
			}
			{
				string s = string("kokos");
				CAGE_TEST(s.size() == 5);
				CAGE_TEST(s[0] == 'k');
			}
			{
				const string str = "lorem ipsum";
				PointerRange<const char> rs = str;
				CAGE_TEST(rs.size() == str.size());
				CAGE_TEST(rs.begin() == str.begin());
			}
		}
		{
			CAGE_TESTCASE("different baseString<N>");
			detail::StringBase<128> a = "ahoj";
			string b = "nazdar";
			detail::StringBase<1024> c = "cau";
			string d = a + b + c;
			CAGE_TEST(d == "ahojnazdarcau");
		}
	}

	void testFunctions()
	{
		{
			CAGE_TESTCASE("reverse");
			CAGE_TEST(reverse(string()) == "");
			CAGE_TEST(reverse(string("ahoj")) == "joha");
			CAGE_TEST(reverse(string("nazdar")) == "radzan");
			CAGE_TEST(reverse(string("omega")) == "agemo");
		}
		{
			CAGE_TESTCASE("tolower, toupper");
			CAGE_TEST(toLower(string("AlMachNa")) == "almachna");
			CAGE_TEST(toUpper(string("AlMachNa")) == "ALMACHNA");
		}
		{
			CAGE_TESTCASE("substring, remove, insert");
			CAGE_TEST(subString(string("almachna"), 2, 4) == "mach");
			CAGE_TEST(subString(string("almachna"), 0, 3) == "alm");
			CAGE_TEST(subString(string("almachna"), 5, 3) == "hna");
			CAGE_TEST(subString(string("almachna"), 0, 8) == "almachna");
			CAGE_TEST(subString(string("almachna"), 5, 5) == "hna");
			CAGE_TEST(subString(string("almachna"), 0, 10) == "almachna");
			CAGE_TEST(subString(string("almachna"), 5, m) == "hna");
			CAGE_TEST(subString(string("almachna"), 10, 3) == "");
			CAGE_TEST(remove(string("almachna"), 2, 4) == "alna");
			CAGE_TEST(insert(string("almachna"), 2, "orangutan") == "alorangutanmachna");
			CAGE_TEST(trim(string(" al ma\nch\tn a\n \t  ")) == "al ma\nch\tn a");
		}
		{
			CAGE_TESTCASE("replace");
			const string c = "ahojnazdar";
			string d = replace(c, 2, 3, "");
			CAGE_TEST(d == "ahazdar");
			CAGE_TEST(d.length() == 7);
			string e = replace(c, 1, 8, "ka");
			CAGE_TEST(e == "akar");
			CAGE_TEST(e.length() == 4);
			CAGE_TEST(replace(c, "ahoj", "nazdar") == "nazdarnazdar");
			CAGE_TEST(replace(c, "a", "zu") == "zuhojnzuzdzur");
			CAGE_TEST(replace(c, "nazdar", "ahoj") == "ahojahoj");
			CAGE_TEST(replace(string("rakokokokodek"), "ko", "") == "radek");
			CAGE_TEST(replace(string("rakokokokodek"), "o", "oo") == "rakookookookoodek");
		}
		{
			CAGE_TESTCASE("split");
			string f = "ab\ne\n\nced\na\n";
			CAGE_TEST(split(f) == "ab");
			CAGE_TEST(f == "e\n\nced\na\n");
			CAGE_TEST(split(f) == "e");
			CAGE_TEST(split(f) == "");
			CAGE_TEST(split(f) == "ced");
			CAGE_TEST(split(f) == "a");
			CAGE_TEST(split(f) == "");
			CAGE_TEST(split(f) == "");
			f = "kulomet";
			CAGE_TEST(split(f, "eu") == "k");
			CAGE_TEST(f == "lomet");
			f = "kulomet";
			CAGE_TEST(split(f, "") == "");
			CAGE_TEST(f == "kulomet");
			CAGE_TEST_ASSERTED(split(f, "za")); // delimiting characters have to be sorted
		}
		{
			CAGE_TESTCASE("find");
			{
				CAGE_TESTCASE("ratata://omega.alt.com/blah/keee/jojo.armagedon");
				const string s = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
				CAGE_TEST(find(s, 'r', 0) == 0);
				CAGE_TEST(find(s, 'a', 1) == 1);
				CAGE_TEST(find(s, 'a', 2) == 3);
				CAGE_TEST(find(s, ':', 0) == 6);
				CAGE_TEST(find(s, ':', 3) == 6);
				CAGE_TEST(find(s, ':', 7) == m);
				CAGE_TEST(find(s, "ta", 0) == 2);
				CAGE_TEST(find(s, "://", 0) == 6);
				CAGE_TEST(find(s, "a", 2) == 3);
				CAGE_TEST(find(s, "ra", 0) == 0);
				CAGE_TEST(find(s, "tata", 0) == 2);
				CAGE_TEST(find(s, "tata", 7) == m);
			}
			{
				CAGE_TESTCASE("0123456789");
				const string s = "0123456789";
				CAGE_TEST(find(s, "35", 0) == m);
				CAGE_TEST(find(s, "45", 0) == 4);
				CAGE_TEST(find(s, "34", 0) == 3);
				CAGE_TEST(find(s, "0", 0) == 0);
				CAGE_TEST(find(s, "89", 0) == 8);
				CAGE_TEST(find(s, "9", 0) == 9);
				CAGE_TEST(find(s, "k", 0) == m);
			}
			{
				CAGE_TESTCASE("finding char only");
				const string s = "0123456789";
				CAGE_TEST(find(s, 'j', 0) == m);
				CAGE_TEST(find(s, '4', 0) == 4);
				CAGE_TEST(find(s, '3', 0) == 3);
				CAGE_TEST(find(s, '0', 0) == 0);
				CAGE_TEST(find(s, '8', 0) == 8);
				CAGE_TEST(find(s, '9', 0) == 9);
				CAGE_TEST(find(s, 'k', 0) == m);
			}
			{
				CAGE_TESTCASE("corner cases");
				const string s = "0123456789";
				CAGE_TEST(find(s, 'a', 100) == m);
				CAGE_TEST(find(s, "a", 100) == m);
				CAGE_TEST(find(s, "abcdefghijklmnopq", 0) == m);
				CAGE_TEST(find(s, "") == m);
				CAGE_TEST(find(s, string(s)) == 0);
				CAGE_TEST(find(string("h"), 'h') == 0);
			}
		}
		{
			CAGE_TESTCASE("trim");
			CAGE_TEST(trim(string("   ori  ")) == "ori");
			CAGE_TEST(trim(string("   ori  "), false, true) == "   ori");
			CAGE_TEST(trim(string("   ori  "), true, false) == "ori  ");
			CAGE_TEST(trim(string("   ori  "), false, false) == "   ori  ");
			CAGE_TEST(trim(string("   ori  "), true, true, " i") == "or");
			CAGE_TEST(trim(string("magma"), true, true, string("am")) == "g");
			CAGE_TEST_ASSERTED(trim(string("magma"), true, true, "za"));
			CAGE_TEST(trim(string("magma"), true, true, "") == "magma");
		}
		{
			CAGE_TESTCASE("pattern");
			CAGE_TEST(isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "", "://", ""));
			CAGE_TEST(isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "rat", "alt", "don"));
			CAGE_TEST(isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "ratata", "://", "armagedon"));
			CAGE_TEST(isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "ratata", "jojo.", "armagedon"));
			CAGE_TEST(!isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "", ":///", ""));
			CAGE_TEST(!isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "http", "", ""));
			CAGE_TEST(!isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "", "", ".cz"));
			CAGE_TEST(!isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "rat", "tata", ""));
			CAGE_TEST(!isPattern(string("ratata://omega.alt.com/blah/keee/jojo.armagedon"), "", "armag", "gedon"));
		}
		{
			CAGE_TESTCASE("url encode and decode");
			CAGE_TEST(encodeUrl(string("for (uint i = 0; i < sts.size (); i++)")) == "for%20(uint%20i%20%3D%200%3B%20i%20%3C%20sts.size%20()%3B%20i++)");
			for (uint32 i = 0; i < 1000; i++)
			{
				string s;
				for (uint32 j = 0, e = randomRange(0, 100); j < e; j++)
				{
					char c = randomRange(0, 255);
					s += string(c);
				}
				string sen = encodeUrl(s);
				string sde = decodeUrl(sen);
				CAGE_TEST(sde == s);
			}
		}
	}

	void testContainers()
	{
		{
			CAGE_TESTCASE("std::map");
			string a = "ahoj";
			string b = "pole";
			std::map<string, uint32> m;
			m[a] = 5;
			m[b] = 10;
			m["ahoj"] = 15;
			CAGE_TEST(m.size() == 2);
			CAGE_TEST(m[a] == 15);
			CAGE_TEST(m[b] == 10);
		}
		{
			CAGE_TESTCASE("hashed string");
			HashString("");
			HashString("1");
			HashString("12");
			HashString("123");
			string a = "hel";
			string b = "lo";
			constexpr uint32 compile_time = HashString("hello");
			string c = a + b;
			uint32 run_time = HashString(c.c_str());
			CAGE_TEST(compile_time == run_time);
			constexpr uint32 different = HashString("different");
			CAGE_TEST(compile_time != different);
		}
	}

	void testConversions()
	{
		CAGE_TESTCASE("conversions");
		test(toFloat(string("0.5")), 0.5f);
		test(toFloat(string("-45.123")), -45.123f);
		test(toFloat(string("-3.8e9")), -3.8e9f);
		test(toDouble(string("0.5")), 0.5);
		test(toDouble(string("-45.123")), -45.123);
		test(toDouble(string("-3.8e9")), -3.8e9);
		test(toDouble(string("-10995.11627776")), -10995.11627776);
		test(toDouble(string("1152.9215046068")), 1152.9215046068);
		test(toDouble(string("21504606846.976")), 21504606846.976);
		test(toDouble(string("-3.8e9")), -3.8e9);
		CAGE_TEST(toUint32(string("157")) == 157);
		CAGE_TEST(toSint32(string("157")) == 157);
		CAGE_TEST(toSint32(string("-157")) == -157);
		CAGE_TEST(toUint64(string("157")) == 157);
		CAGE_TEST(toUint64(string("50000000000")) == 50000000000);
		CAGE_TEST(toSint64(string("-50000000000")) == -50000000000);
		CAGE_TEST(toSint64(string("157")) == 157);
		CAGE_TEST(toSint64(string("-157")) == -157);
		CAGE_TEST(toBool(string("yES")));
		CAGE_TEST(toBool(string("T")));
		CAGE_TEST(!toBool(string("FALse")));
		CAGE_TEST(isBool(string("yES")));
		CAGE_TEST(isBool(string("T")));
		CAGE_TEST(isBool(string("FALse")));
		CAGE_TEST(!isBool(string("kk")));
		CAGE_TEST(isDigitsOnly(string()));
		CAGE_TEST(isDigitsOnly(string("157")));
		CAGE_TEST(!isDigitsOnly(string("hola")));
		CAGE_TEST_THROWN(toBool(string("")));
		CAGE_TEST_THROWN(toFloat(string("")));
		CAGE_TEST_THROWN(toSint32(string("")));
		CAGE_TEST_THROWN(toSint64(string("")));
		CAGE_TEST_THROWN(toUint32(string("")));
		CAGE_TEST_THROWN(toUint64(string("")));
		CAGE_TEST_THROWN(toBool(string("hola")));
		CAGE_TEST_THROWN(toFloat(string("hola")));
		CAGE_TEST_THROWN(toSint32(string("hola")));
		CAGE_TEST_THROWN(toSint64(string("hola")));
		CAGE_TEST_THROWN(toUint32(string("hola")));
		CAGE_TEST_THROWN(toUint64(string("hola")));
		CAGE_TEST_THROWN(toSint32(string("157hola")));
		CAGE_TEST_THROWN(toSint64(string("157hola")));
		CAGE_TEST_THROWN(toUint32(string("157hola")));
		CAGE_TEST_THROWN(toUint64(string("157hola")));
		CAGE_TEST_THROWN(toSint32(string("15.7")));
		CAGE_TEST_THROWN(toSint64(string("15.7")));
		CAGE_TEST_THROWN(toUint32(string("15.7")));
		CAGE_TEST_THROWN(toUint64(string("15.7")));
		CAGE_TEST_THROWN(toUint32(string("-157")));
		CAGE_TEST_THROWN(toUint64(string("-157")));
		CAGE_TEST_THROWN(toFloat(string("-3.8ha")));
		CAGE_TEST_THROWN(toFloat(string("-3.8e-4ha")));
		CAGE_TEST_THROWN(toSint32(string("0x157")));
		CAGE_TEST_THROWN(toSint64(string("0x157")));
		CAGE_TEST_THROWN(toUint32(string("0x157")));
		CAGE_TEST_THROWN(toUint64(string("0x157")));
		CAGE_TEST_THROWN(toSint32(string(" 157")));
		CAGE_TEST_THROWN(toSint64(string(" 157")));
		CAGE_TEST_THROWN(toUint32(string(" 157")));
		CAGE_TEST_THROWN(toUint64(string(" 157")));
		CAGE_TEST_THROWN(toFloat(string(" 157")));
		CAGE_TEST_THROWN(toSint32(string("157 ")));
		CAGE_TEST_THROWN(toSint64(string("157 ")));
		CAGE_TEST_THROWN(toUint32(string("157 ")));
		CAGE_TEST_THROWN(toUint64(string("157 ")));
		CAGE_TEST_THROWN(toFloat(string("157 ")));
		CAGE_TEST_THROWN(toBool(string(" true")));
		CAGE_TEST_THROWN(toBool(string("true ")));
		CAGE_TEST_THROWN(toBool(string("tee")));
		CAGE_TEST_THROWN(toSint32(string("50000000000")));
		CAGE_TEST_THROWN(toUint32(string("50000000000")));

		{
			CAGE_TESTCASE("long long numbers and toSint64()");
			CAGE_TEST(toSint64(string("-1099511627776")) == -1099511627776);
			CAGE_TEST_THROWN(toUint64(string("-1099511627776")));
			CAGE_TEST(toSint64(string("1152921504606846976")) == 1152921504606846976);
			CAGE_TEST(toUint64(string("1152921504606846976")) == 1152921504606846976);
		}
		{
			CAGE_TESTCASE("isInteger");
			CAGE_TEST(isDigitsOnly(string("")));
			CAGE_TEST(isInteger(string("5")));
			CAGE_TEST(isDigitsOnly(string("5")));
			CAGE_TEST(isInteger(string("123")));
			CAGE_TEST(isInteger(string("-123")));
			CAGE_TEST(!isInteger(string("")));
			CAGE_TEST(!isInteger(string("pet")));
			CAGE_TEST(!isInteger(string("13.42")));
			CAGE_TEST(!isInteger(string("13k")));
			CAGE_TEST(!isInteger(string("k13")));
			CAGE_TEST(!isInteger(string("1k3")));
			CAGE_TEST(!isInteger(string("-")));
			CAGE_TEST(!isInteger(string("+")));
			CAGE_TEST(!isDigitsOnly(string("-123")));
			CAGE_TEST(!isInteger(string("+123")));
		}
		{
			CAGE_TESTCASE("isReal");
			CAGE_TEST(isReal(string("5")));
			CAGE_TEST(isReal(string("13.42")));
			CAGE_TEST(isReal(string("-13.42")));
			CAGE_TEST(!isReal(string("")));
			CAGE_TEST(!isReal(string("pet")));
			CAGE_TEST(!isReal(string("13,42")));
			CAGE_TEST(!isReal(string("13.42k")));
			CAGE_TEST(!isReal(string("+13.42")));
			CAGE_TEST(!isReal(string("13.k42")));
			CAGE_TEST(!isReal(string("13k.42")));
			CAGE_TEST_THROWN(toSint32(string("ha")));
		}
		{
			CAGE_TESTCASE("float -> string -> float");
			for (float a : { 123.456f, 1056454.4f, 0.000045678974f, -45544.f, 0.18541e-10f })
			{
				const string s = stringizer() + a;
				float b = toFloat(s);
				test(a, b);
			}
		}
		{
			CAGE_TESTCASE("double -> string -> double");
			for (double a : { 123.456789, 1056454.42189, 0.0000456789741248145, -45524144.0254, 0.141158541e-10 })
			{
				const string s = stringizer() + a;
				double b = toDouble(s);
				test(a, b);
			}
		}
	}

	void testCopies1()
	{
		CAGE_TESTCASE("copies and changes 1 - just operators");
		string a = "ahoj";
		string b = "nazdar";
		string c = "pokus";
		string d = "omega";
		string e = a + b;
		string f = b + c + d;
		string g = a;
		a = "opice";
		b = "lapis";
		c = "";
		CAGE_TEST(a == "opice");
		CAGE_TEST(b == "lapis");
		CAGE_TEST(c == "");
		CAGE_TEST(d == "omega");
		CAGE_TEST(e == "ahojnazdar");
		CAGE_TEST(f == "nazdarpokusomega");
		CAGE_TEST(g == "ahoj");
		a = b;
		b = c;
		d = e;
		CAGE_TEST(a == "lapis");
		CAGE_TEST(b == "");
		CAGE_TEST(c == "");
		CAGE_TEST(d == "ahojnazdar");
	}

	void testCopies2()
	{
		CAGE_TESTCASE("copies and changes 2 - with methods");
		string a = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
		string d, p, f, e;
		pathDecompose(a, d, p, f, e);
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		reverse(a);
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		toLower(a);
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		toUpper(a);
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");

		{
			CAGE_TESTCASE("substring");
			string sub = subString(a, 5, 7);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = subString(a, 2, 15);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = subString(a, 15, 50);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = subString(a, 0, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = subString(a, 17, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = subString(a, -5, 12);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("remove");
			string rem = remove(a, 5, 7);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = remove(a, 2, 15);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = remove(a, 15, 50);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = remove(a, 0, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = remove(a, 17, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = remove(a, -5, 12);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("insert");
			string ins = insert(a, 5, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = insert(a, 2, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = insert(a, 15, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = insert(a, 0, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = insert(a, 17, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = insert(a, -5, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("replace");
			string rep = replace(a, 5, 12, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = replace(a, 2, 5, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = replace(a, 15, -3, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = replace(a, "om", "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = replace(a, "tra", "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = replace(a, "", "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("replace is non-recursive");
			string s = "abacab";
			string r = replace(s, "a", "aa");
			CAGE_TEST(r == "aabaacaab");
			string k = replace(r, "aa", "a");
			CAGE_TEST(k == s);
		}
		{
			CAGE_TESTCASE("split");
			string a = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
			CAGE_TEST(split(a, "/") == "ratata:");
			CAGE_TEST(a == "/omega.alt.com/blah/keee/jojo.armagedon");
			CAGE_TEST(split(a, "/") == "");
			CAGE_TEST(a == "omega.alt.com/blah/keee/jojo.armagedon");
			CAGE_TEST(split(a, "/") == "omega.alt.com");
			CAGE_TEST(a == "blah/keee/jojo.armagedon");
			CAGE_TEST(split(a, "/") == "blah");
			CAGE_TEST(a == "keee/jojo.armagedon");
			CAGE_TEST(split(a, "/") == "keee");
			CAGE_TEST(a == "jojo.armagedon");
			CAGE_TEST(split(a, "/") == "jojo.armagedon");
			CAGE_TEST(a == "");
		}
	}

	struct Custom
	{
		int value;
		explicit Custom(int value = 0) : value(value)
		{}
	};

	template<uint32 N>
	detail::StringizerBase<N> &operator + (detail::StringizerBase<N> &str, const Custom &other)
	{
		return str + other.value;
	}

	void functionTakingString(const string &str)
	{}

	void testStringizer()
	{
		CAGE_TESTCASE("stringizer");
		{
			CAGE_TESTCASE("conversions");
			uint8  ui8 = 1;
			uint16 ui16 = 2;
			uint32 ui32 = 3;
			uint64 ui64 = 4;
			sint8  si8 = 5;
			sint16 si16 = 6;
			sint32 si32 = 7;
			sint64 si64 = 8;
			bool bt = true;
			bool bf = false;
			CAGE_TEST(string(stringizer() + "begin" + ui8 + ui16 + ui32 + ui64 + "s" + si8 + si16 + si32 + si64 + "b" + bt + bf + "end") == "begin1234s5678btruefalseend");
		}
		{
			CAGE_TESTCASE("r-value stringizer");
			{
				string str = stringizer() + 123 + "abc" + 456;
			}
			{
				Custom custom(42);
				string str = stringizer() + custom;
			}
			{
				functionTakingString(stringizer() + 123);
			}
		}
		{
			CAGE_TESTCASE("l-value stringizer");
			{
				stringizer s;
				string str = s + 123 + "abc" + 456;
			}
			{
				Custom custom(42);
				stringizer s;
				string str = s + custom;
			}
			{
				stringizer s;
				functionTakingString(s + 123);
			}
		}
		{
			CAGE_TESTCASE("pointers");
			CAGE_TEST(string(stringizer() + "hi") == "hi");
			int obj = 42;
			int *ptr = &obj;
			CAGE_TEST(toUint64(string(stringizer() + ptr)) == (uint64)ptr);
		}
	}
}

void testStrings()
{
	CAGE_TESTCASE("strings");
	testStringLiterals();
	testBasics();
	testFunctions();
	testContainers();
	testConversions();
	testCopies1();
	testCopies2();
	testStringizer();
}
