#include <cstring>
#include <map>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/files.h>
#include <cage-core/hashString.h>

namespace
{
	void testBasics()
	{
		{
			CAGE_TESTCASE("constructors and operator +");
			CAGE_TEST(string("ra") + "ke" + "ta" == "raketa");
			bool b = true;
			CAGE_TEST(string(b) == "true");
			int i1 = -123;
			CAGE_TEST(string(i1) == "-123");
			unsigned int i2 = 123;
			CAGE_TEST(string(i2) == "123");
			sint8 i3 = 123;
			CAGE_TEST(string(i3) == "123");
			sint16 i4 = 123;
			CAGE_TEST(string(i4) == "123");
			sint32 i5 = 123;
			CAGE_TEST(string(i5) == "123");
			sint64 i6 = 123;
			CAGE_TEST(string(i6) == "123");
			uint8 i7 = 123;
			CAGE_TEST(string(i7) == "123");
			uint16 i8 = 123;
			CAGE_TEST(string(i8) == "123");
			uint32 i9 = 123;
			CAGE_TEST(string(i9) == "123");
			uint64 i10 = 123;
			CAGE_TEST(string(i10) == "123");
			float f = 5;
			CAGE_TEST(string(f) == "5.000000");
			double d = 5;
			CAGE_TEST(string(d) == "5.000000");
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
			CAGE_TESTCASE("reverse");
			CAGE_TEST(string().reverse() == "");
			CAGE_TEST(string("ahoj").reverse() == "joha");
			CAGE_TEST(string("nazdar").reverse() == "radzan");
			CAGE_TEST(string("omega").reverse() == "agemo");
		}
		{
			CAGE_TESTCASE("tolower, toupper");
			CAGE_TEST(string("AlMachNa").toLower() == "almachna");
			CAGE_TEST(string("AlMachNa").toUpper() == "ALMACHNA");
		}
		{
			CAGE_TESTCASE("substring, remove, insert");
			CAGE_TEST(string("almachna").subString(2, 4) == "mach");
			CAGE_TEST(string("almachna").subString(0, 3) == "alm");
			CAGE_TEST(string("almachna").subString(5, 3) == "hna");
			CAGE_TEST(string("almachna").subString(0, 8) == "almachna");
			CAGE_TEST(string("almachna").remove(2, 4) == "alna");
			CAGE_TEST(string("almachna").insert(2, "orangutan") == "alorangutanmachna");
			CAGE_TEST(string(" al ma\nch\tn a\n \t  ").trim() == "al ma\nch\tn a");
		}
		{
			CAGE_TESTCASE("replace");
			const string c = "ahojnazdar";
			string d = c.replace(2, 3, "");
			CAGE_TEST(d == "ahazdar");
			CAGE_TEST(d.length() == 7);
			string e = c.replace(1, 8, "ka");
			CAGE_TEST(e == "akar");
			CAGE_TEST(e.length() == 4);
			CAGE_TEST(c.replace("ahoj", "nazdar") == "nazdarnazdar");
			CAGE_TEST(c.replace("a", "zu") == "zuhojnzuzdzur");
			CAGE_TEST(c.replace("nazdar", "ahoj") == "ahojahoj");
			CAGE_TEST(string("rakokokokodek").replace("ko", "") == "radek");
			CAGE_TEST(string("rakokokokodek").replace("o", "oo") == "rakookookookoodek");
		}
		{
			CAGE_TESTCASE("split");
			string f = "ab\ne\n\nced\na\n";
			CAGE_TEST(f.split() == "ab");
			CAGE_TEST(f == "e\n\nced\na\n");
			CAGE_TEST(f.split() == "e");
			CAGE_TEST(f.split() == "");
			CAGE_TEST(f.split() == "ced");
			CAGE_TEST(f.split() == "a");
			CAGE_TEST(f.split() == "");
			CAGE_TEST(f.split() == "");
			f = "kulomet";
			CAGE_TEST(f.split("eu") == "k");
			CAGE_TEST(f == "lomet");
			f = "kulomet";
			CAGE_TEST(f.split("") == "");
			CAGE_TEST(f == "kulomet");
			CAGE_TEST_ASSERTED(f.split("za"));
		}
		{
			CAGE_TESTCASE("conversions");
			CAGE_TEST(real(string("0.5").toFloat()) == (real)0.5f);
			CAGE_TEST(real(string("-45.123").toFloat()) == (real)-45.123f);
			CAGE_TEST(real(string("-3.8e9").toFloat()) == (real)-3.8e9f);
			CAGE_TEST(string("157").toUint32() == 157);
			CAGE_TEST(string("+157").toUint32() == 157);
			CAGE_TEST(string("157").toSint32() == 157);
			CAGE_TEST(string("+157").toSint32() == 157);
			CAGE_TEST(string("-157").toSint32() == -157);
			CAGE_TEST(string("157").toUint64() == 157);
			CAGE_TEST(string("50000000000").toUint64() == 50000000000);
			CAGE_TEST(string("-50000000000").toSint64() == -50000000000);
			CAGE_TEST(string("157").toSint64() == 157);
			CAGE_TEST(string("-157").toSint64() == -157);
			CAGE_TEST(string("yES").toBool());
			CAGE_TEST(string("T").toBool());
			CAGE_TEST(!string("FALse").toBool());
			CAGE_TEST(string().isDigitsOnly());
			CAGE_TEST(string("157").isDigitsOnly());
			CAGE_TEST(!string("hola").isDigitsOnly());
			CAGE_TEST_THROWN(string("").toBool());
			CAGE_TEST_THROWN(string("").toFloat());
			CAGE_TEST_THROWN(string("").toSint32());
			CAGE_TEST_THROWN(string("").toSint64());
			CAGE_TEST_THROWN(string("").toUint32());
			CAGE_TEST_THROWN(string("").toUint64());
			CAGE_TEST_THROWN(string("hola").toBool());
			CAGE_TEST_THROWN(string("hola").toFloat());
			CAGE_TEST_THROWN(string("hola").toSint32());
			CAGE_TEST_THROWN(string("hola").toSint64());
			CAGE_TEST_THROWN(string("hola").toUint32());
			CAGE_TEST_THROWN(string("hola").toUint64());
			CAGE_TEST_THROWN(string("157hola").toSint32());
			CAGE_TEST_THROWN(string("157hola").toSint64());
			CAGE_TEST_THROWN(string("157hola").toUint32());
			CAGE_TEST_THROWN(string("157hola").toUint64());
			CAGE_TEST_THROWN(string("15.7").toSint32());
			CAGE_TEST_THROWN(string("15.7").toSint64());
			CAGE_TEST_THROWN(string("15.7").toUint32());
			CAGE_TEST_THROWN(string("15.7").toUint64());
			CAGE_TEST_THROWN(string("-157").toUint32());
			CAGE_TEST_THROWN(string("-157").toUint64());
			CAGE_TEST_THROWN(string("-3.8ha").toFloat());
			CAGE_TEST_THROWN(string("-3.8e-4ha").toFloat());
			CAGE_TEST_THROWN(string("0x157").toSint32());
			CAGE_TEST_THROWN(string("0x157").toSint64());
			CAGE_TEST_THROWN(string("0x157").toUint32());
			CAGE_TEST_THROWN(string("0x157").toUint64());
			CAGE_TEST_THROWN(string(" 157").toSint32());
			CAGE_TEST_THROWN(string(" 157").toSint64());
			CAGE_TEST_THROWN(string(" 157").toUint32());
			CAGE_TEST_THROWN(string(" 157").toUint64());
			CAGE_TEST_THROWN(string("157 ").toSint32());
			CAGE_TEST_THROWN(string("157 ").toSint64());
			CAGE_TEST_THROWN(string("157 ").toUint32());
			CAGE_TEST_THROWN(string("157 ").toUint64());
			CAGE_TEST_THROWN(string(" 157").toFloat());
			CAGE_TEST_THROWN(string("157 ").toFloat());
			CAGE_TEST_THROWN(string(" true").toBool());
			CAGE_TEST_THROWN(string("true ").toBool());
			CAGE_TEST_THROWN(string("tee").toBool());
			CAGE_TEST_THROWN(string("50000000000").toSint32());
			CAGE_TEST_THROWN(string("50000000000").toUint32());

			{
				CAGE_TESTCASE("long long numbers and toInt64()");
				CAGE_TEST(string("-1099511627776").toSint64() == -1099511627776);
				CAGE_TEST(string("1152921504606846976").toSint64() == 1152921504606846976);
			}
			{
				CAGE_TESTCASE("isreal");
				CAGE_TEST(string("5").isReal(true));
				CAGE_TEST(!string("pet").isReal(true));
				CAGE_TEST_THROWN(string("ha").toSint32());
			}
		}
		{
			CAGE_TESTCASE("find");
			{
				CAGE_TESTCASE("ratata://omega.alt.com/blah/keee/jojo.armagedon");
				const string s = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
				CAGE_TEST(s.find('r', 0) == 0);
				CAGE_TEST(s.find('a', 1) == 1);
				CAGE_TEST(s.find('a', 2) == 3);
				CAGE_TEST(s.find(':', 0) == 6);
				CAGE_TEST(s.find(':', 3) == 6);
				CAGE_TEST(s.find(':', 7) == m);
				CAGE_TEST(s.find("ta", 0) == 2);
				CAGE_TEST(s.find("://", 0) == 6);
				CAGE_TEST(s.find("a", 2) == 3);
				CAGE_TEST(s.find("ra", 0) == 0);
				CAGE_TEST(s.find("tata", 0) == 2);
				CAGE_TEST(s.find("tata", 7) == m);
			}
			{
				CAGE_TESTCASE("0123456789");
				const string s = "0123456789";
				CAGE_TEST(s.find("35", 0) == m);
				CAGE_TEST(s.find("45", 0) == 4);
				CAGE_TEST(s.find("34", 0) == 3);
				CAGE_TEST(s.find("0", 0) == 0);
				CAGE_TEST(s.find("89", 0) == 8);
				CAGE_TEST(s.find("9", 0) == 9);
				CAGE_TEST(s.find("k", 0) == m);
			}
			{
				CAGE_TESTCASE("finding char only");
				const string s = "0123456789";
				CAGE_TEST(s.find('j', 0) == m);
				CAGE_TEST(s.find('4', 0) == 4);
				CAGE_TEST(s.find('3', 0) == 3);
				CAGE_TEST(s.find('0', 0) == 0);
				CAGE_TEST(s.find('8', 0) == 8);
				CAGE_TEST(s.find('9', 0) == 9);
				CAGE_TEST(s.find('k', 0) == m);
			}
			{
				CAGE_TESTCASE("corner cases");
				const string s = "0123456789";
				CAGE_TEST(s.find('a', 100) == m);
				CAGE_TEST(s.find("a", 100) == m);
				CAGE_TEST(s.find("abcdefghijklmnopq", 0) == m);
				CAGE_TEST(s.find("") == m);
				CAGE_TEST(s.find(s) == 0);
				CAGE_TEST(string("h").find('h') == 0);
			}
		}
		{
			CAGE_TESTCASE("trim");
			CAGE_TEST(string("   ori  ").trim() == "ori");
			CAGE_TEST(string("   ori  ").trim(false, true) == "   ori");
			CAGE_TEST(string("   ori  ").trim(true, false) == "ori  ");
			CAGE_TEST(string("   ori  ").trim(false, false) == "   ori  ");
			CAGE_TEST(string("   ori  ").trim(true, true, " i") == "or");
			CAGE_TEST(string("magma").trim(true, true, string("am")) == "g");
			CAGE_TEST_ASSERTED(string("magma").trim(true, true, "za"));
			CAGE_TEST(string("magma").trim(true, true, "") == "magma");
		}
		{
			CAGE_TESTCASE("pattern");
			CAGE_TEST(string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("", "://", ""));
			CAGE_TEST(string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("rat", "alt", "don"));
			CAGE_TEST(string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("ratata", "://", "armagedon"));
			CAGE_TEST(string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("ratata", "jojo.", "armagedon"));
			CAGE_TEST(!string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("", ":///", ""));
			CAGE_TEST(!string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("http", "", ""));
			CAGE_TEST(!string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("", "", ".cz"));
			CAGE_TEST(!string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("rat", "tata", ""));
			CAGE_TEST(!string("ratata://omega.alt.com/blah/keee/jojo.armagedon").isPattern("", "armag", "gedon"));
		}
		{
			CAGE_TESTCASE("url encode and decode");
			CAGE_TEST(string("for (uint i = 0; i < sts.size (); i++)").encodeUrl() == "for%20(uint%20i%20%3D%200%3B%20i%20%3C%20sts.size%20()%3B%20i++)");
			for (uint32 i = 0; i < 1000; i++)
			{
				string s;
				for (uint32 j = 0, e = randomRange(0, 100); j < e; j++)
				{
					char c = randomRange(0, 255);
					s += string(&c, 1);
				}
				string sen = s.encodeUrl();
				string sde = sen.decodeUrl();
				CAGE_TEST(sde == s);
			}
		}
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
			CAGE_TESTCASE("different baseString<N>");
			detail::stringBase<128> a = "ahoj";
			string b = "nazdar";
			detail::stringBase<1024> c = "cau";
			string d = a + b + c;
			CAGE_TEST(d == "ahojnazdarcau");
		}
		{
			CAGE_TESTCASE("hashed string");
			hashString("");
			hashString("1");
			hashString("12");
			hashString("123");
			string a = "hel";
			string b = "lo";
			uint32 compile_time = hashString("hello");
			string c = a + b;
			uint32 run_time = hashString(c.c_str());
			CAGE_TEST(compile_time == run_time);
			uint32 different = hashString("different");
			CAGE_TEST(compile_time != different);
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
		a.reverse();
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		a.toLower();
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		a.toUpper();
		CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");

		{
			CAGE_TESTCASE("substring");
			string sub = a.subString(5, 7);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = a.subString(2, 15);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = a.subString(15, 50);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = a.subString(0, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = a.subString(17, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			sub = a.subString(-5, 12);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("remove");
			string rem = a.remove(5, 7);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = a.remove(2, 15);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = a.remove(15, 50);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = a.remove(0, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = a.remove(17, m);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rem = a.remove(-5, 12);
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("insert");
			string ins = a.insert(5, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = a.insert(2, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = a.insert(15, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = a.insert(0, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = a.insert(17, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			ins = a.insert(-5, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("replace");
			string rep = a.replace(5, 12, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = a.replace(2, 5, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = a.replace(15, -3, "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = a.replace("om", "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = a.replace("tra", "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
			rep = a.replace("", "pomeranc");
			CAGE_TEST(a == "ratata://omega.alt.com/blah/keee/jojo.armagedon");
		}
		{
			CAGE_TESTCASE("split");
			string a = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
			CAGE_TEST(a.split("/") == "ratata:");
			CAGE_TEST(a == "/omega.alt.com/blah/keee/jojo.armagedon");
			CAGE_TEST(a.split("/") == "");
			CAGE_TEST(a == "omega.alt.com/blah/keee/jojo.armagedon");
			CAGE_TEST(a.split("/") == "omega.alt.com");
			CAGE_TEST(a == "blah/keee/jojo.armagedon");
			CAGE_TEST(a.split("/") == "blah");
			CAGE_TEST(a == "keee/jojo.armagedon");
			CAGE_TEST(a.split("/") == "keee");
			CAGE_TEST(a == "jojo.armagedon");
			CAGE_TEST(a.split("/") == "jojo.armagedon");
			CAGE_TEST(a == "");
		}
	}

	void testFilePaths()
	{
		CAGE_TESTCASE("file paths");
		{
			CAGE_TESTCASE("path decompose");
			string s = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
			string d, p, f, e;
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "ratata");
			CAGE_TEST(p == "//omega.alt.com/blah/keee");
			CAGE_TEST(f == "jojo");
			CAGE_TEST(e == ".armagedon");
			s = "jup.h";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "");
			CAGE_TEST(f == "jup");
			CAGE_TEST(e == ".h");
			s = ".htaccess";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "");
			CAGE_TEST(f == "");
			CAGE_TEST(e == ".htaccess");
			s = "rumprsteak";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "");
			CAGE_TEST(f == "rumprsteak");
			CAGE_TEST(e == "");
			s = "algoritmus/astar";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "algoritmus");
			CAGE_TEST(f == "astar");
			CAGE_TEST(e == "");
			s = "au.ro.ra";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "");
			CAGE_TEST(f == "au.ro");
			CAGE_TEST(e == ".ra");
			s = "ratata:/omega";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "ratata");
			CAGE_TEST(p == "/");
			CAGE_TEST(f == "omega");
			CAGE_TEST(e == "");
			s = "..";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "..");
			CAGE_TEST(f == "");
			CAGE_TEST(e == "");
			s = "a/..";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "a/..");
			CAGE_TEST(f == "");
			CAGE_TEST(e == "");
			s = ".";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == ".");
			CAGE_TEST(f == "");
			CAGE_TEST(e == "");
			s = "a/.";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "a/.");
			CAGE_TEST(f == "");
			CAGE_TEST(e == "");
			s = ":";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "/");
			CAGE_TEST(f == "");
			CAGE_TEST(e == "");
			s = ":pes";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "/");
			CAGE_TEST(f == "pes");
			CAGE_TEST(e == "");
			s = "/path/path/path/file.ext";
			pathDecompose(s, d, p, f, e);
			CAGE_TEST(d == "");
			CAGE_TEST(p == "/path/path/path");
			CAGE_TEST(f == "file");
			CAGE_TEST(e == ".ext");
			s = "ratata://omega.alt.com/blah/keee/jojo.armagedon";
			CAGE_TEST(pathExtractPath(s) == "ratata://omega.alt.com/blah/keee");
			CAGE_TEST(pathExtractDrive(s) == "ratata");
			CAGE_TEST(pathExtractPathNoDrive(s) == "/omega.alt.com/blah/keee");
			CAGE_TEST(pathExtractFilename(s) == "jojo.armagedon");
			CAGE_TEST(pathExtractFilenameNoExtension(s) == "jojo");
			CAGE_TEST(pathExtractExtension(s) == ".armagedon");
		}
		{
			CAGE_TESTCASE("absolute and relative paths");
			// absolute paths
			CAGE_TEST(pathIsAbs("k:/"));
			CAGE_TEST(pathIsAbs("k:/a"));
			CAGE_TEST(pathIsAbs("k:/a/b"));
			CAGE_TEST(pathIsAbs("k://"));
			CAGE_TEST(pathIsAbs("k://a"));
			CAGE_TEST(pathIsAbs("k://a/b"));
			CAGE_TEST(pathIsAbs("/"));
			CAGE_TEST(pathIsAbs("/a"));
			CAGE_TEST(pathIsAbs("/a/b"));
			CAGE_TEST(pathIsAbs("lol:/omg"));
			// relative paths
			CAGE_TEST(!pathIsAbs(""));
			CAGE_TEST(!pathIsAbs("a"));
			CAGE_TEST(!pathIsAbs("a/b"));
			CAGE_TEST(!pathIsAbs("."));
			CAGE_TEST(!pathIsAbs(".."));
			CAGE_TEST(!pathIsAbs("../.."));
		}
		{
			CAGE_TESTCASE("path simplifications");
			CAGE_TEST(pathSimplify("a//b") == "a/b");
			CAGE_TEST(pathSimplify("a\\b") == "a/b");
			CAGE_TEST(pathSimplify("a/./b") == "a/b");
			CAGE_TEST(pathSimplify("a/b/c/../d") == "a/b/d");
			CAGE_TEST(pathSimplify("a/") == "a");
			CAGE_TEST(pathSimplify("/a") == "/a");
			CAGE_TEST(pathSimplify("/") == "/");
			CAGE_TEST(pathSimplify("//") == "/");
			CAGE_TEST(pathSimplify("///") == "/");
			CAGE_TEST(pathSimplify("u:/") == "u:/");
			CAGE_TEST(pathSimplify("u:") == "u:/");
			CAGE_TEST(pathSimplify("u:/abc") == "u:/abc");
			CAGE_TEST(pathSimplify(":") == "/");
			CAGE_TEST(pathSimplify(":/abc") == "/abc");
			CAGE_TEST(pathSimplify("://abc") == "/abc");
			CAGE_TEST(pathSimplify("a/..") == "");
			CAGE_TEST(pathSimplify("a/.") == "a");
			CAGE_TEST(pathSimplify("./") == "");
			CAGE_TEST(pathSimplify("./a") == "a");
			CAGE_TEST(pathSimplify(".a") == ".a");
			CAGE_TEST(pathSimplify("a.") == "a.");
			CAGE_TEST(pathSimplify("b/.a") == "b/.a");
			CAGE_TEST(pathSimplify("b/a.") == "b/a.");
			CAGE_TEST(pathSimplify(".a/b") == ".a/b");
			CAGE_TEST(pathSimplify("a./b") == "a./b");
			CAGE_TEST(pathSimplify("a/./././b") == "a/b");
			CAGE_TEST(pathSimplify("../../../a") == "../../../a");
			CAGE_TEST_THROWN(pathSimplify("/.."));
			CAGE_TEST_THROWN(pathSimplify("/./.."));
			CAGE_TEST_THROWN(pathSimplify("/a/../.."));
		}
		{
			CAGE_TESTCASE("path joins");
			CAGE_TEST(pathJoin("", "") == "");
			CAGE_TEST(pathJoin("abc", "") == "abc");
			CAGE_TEST(pathJoin("", "abc") == "abc");
			CAGE_TEST(pathJoin("abc", "def") == "abc/def");
			CAGE_TEST(pathJoin("abc", "def/") == "abc/def");
			CAGE_TEST(pathJoin("abc/", "def") == "abc/def");
			CAGE_TEST(pathJoin("/a", "b") == "/a/b");
			CAGE_TEST(pathJoin("a/", "b") == "a/b");
			CAGE_TEST(pathJoin("c:/a", "b") == "c:/a/b");
			CAGE_TEST(pathJoin("a/b/c", "d/e/f") == "a/b/c/d/e/f");
			CAGE_TEST(pathJoin(".", "a") == "a");
			CAGE_TEST(pathJoin("a", ".") == "a");
			CAGE_TEST(pathJoin("a", "..") == "");
			CAGE_TEST(pathJoin(".", ".") == "");
			CAGE_TEST(pathJoin("abc/../def", "ghi") == "def/ghi");
			CAGE_TEST(pathJoin("/abc/../def", "ghi") == "/def/ghi");
			CAGE_TEST(pathJoin("abc/./def", "ghi") == "abc/def/ghi");
			CAGE_TEST(pathJoin("../abc", "def") == "../abc/def");
			CAGE_TEST(pathJoin("./abc", "def") == "abc/def");
			CAGE_TEST(pathJoin("a/b/c/../../d", "e") == "a/d/e");
			CAGE_TEST(pathJoin("a/b/c/../../../../d", "e") == "../d/e");
			CAGE_TEST(pathJoin("a", "b/c/../../d") == "a/d");
			CAGE_TEST(pathJoin("a", "b/c/../../../../d") == "../d");
			CAGE_TEST(pathJoin("k:/a", "..") == "k:/");
			CAGE_TEST(pathJoin("k://a", "..") == "k:/");
			CAGE_TEST(pathJoin("/a", "..") == "/");
			CAGE_TEST(pathJoin("../../a/b", "c") == "../../a/b/c");
			CAGE_TEST(pathJoin("", "ab") == "ab");
			CAGE_TEST(pathJoin("ab", "") == "ab");
			CAGE_TEST(pathJoin("/ab", "") == "/ab");
			CAGE_TEST_THROWN(pathJoin("/a", "../.."));
			CAGE_TEST_THROWN(pathJoin("a", "/a"));
			CAGE_TEST_THROWN(pathJoin("a", "c:/"));
			CAGE_TEST_THROWN(pathJoin("/", ".."));
			CAGE_TEST_THROWN(pathJoin(".", "/"));
			CAGE_TEST_THROWN(pathJoin("k:/", ".."));
			CAGE_TEST_THROWN(pathJoin("k://", ".."));
			CAGE_TEST_THROWN(pathJoin("/", ".."));
			CAGE_TEST_THROWN(pathJoin("", "/ab"));
		}
		{
			CAGE_TESTCASE("path to relative");
			CAGE_TEST(pathToRel("") == "");
			CAGE_TEST(pathToRel("abc") == "abc");
			CAGE_TEST(pathToRel("..") == "..");
			CAGE_TEST(pathToRel("lol:/omg") == "lol:/omg");
			CAGE_TEST(pathToRel("lol://omg") == "lol:/omg");
			CAGE_TEST(pathToRel(pathWorkingDir()) == "");
			CAGE_TEST(pathToRel(pathJoin(pathWorkingDir(), "abc")) == "abc");
			CAGE_TEST(pathToRel(pathJoin(pathWorkingDir(), "abc"), pathJoin(pathWorkingDir(), "abc")) == "");
		}
		{
			CAGE_TESTCASE("path validity");
			CAGE_TEST(pathIsValid("hi-Jane"));
			CAGE_TEST(pathIsValid("/peter/pan"));
			CAGE_TEST(pathIsValid("proto:/path1/path2/file.ext")); // this path must be considered valid even on windows

#ifdef CAGE_SYSTEM_WINDOWS
			// these file names are invalid on windows, but are valid on linux
			// this makes testing a lot more complex.
			// for example, it would make sense to allow "windows forbidden characters" inside archives even on windows, since these limitations do not apply to archives
			//   but the function cannot possibly know where is the path going to be used
			CAGE_TEST(!pathIsValid("a:b:c"));
			CAGE_TEST(!pathIsValid("a/b:c"));
			CAGE_TEST(!pathIsValid("/a:b"));
#endif // CAGE_SYSTEM_WINDOWS
		}
	}
}

void testStrings()
{
	CAGE_TESTCASE("strings");
	testBasics();
	testCopies1();
	testCopies2();
	testFilePaths();
}
