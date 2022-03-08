#include "main.h"
#include <cage-core/files.h>
#include <cage-core/ini.h>

#include <vector>
#include <algorithm>

namespace
{
	void testVectors(std::vector<String> a, std::vector<String> b)
	{
		CAGE_TEST(a.size() == b.size());
		std::sort(a.begin(), a.end());
		std::sort(b.begin(), b.end());
		for (uint32 i = 0; i < a.size(); i++)
			CAGE_TEST(a[i] == b[i]);
	}

	std::vector<String> vectorize(const Holder<PointerRange<String>> &r)
	{
		return std::vector<String>(r->begin(), r->end());
	}
}

void testConfigIni()
{
	CAGE_TESTCASE("config ini");

	{
		CAGE_TESTCASE("basic ini");
		Holder<Ini> ini = newIni();
		ini->setString("section", "item", "value");
		CAGE_TEST(ini->getString("section", "item", "default") == "value");
		CAGE_TEST(ini->getString("section", "non-item", "default") == "default");
		CAGE_TEST(ini->sectionExists("section"));
		CAGE_TEST(!ini->sectionExists("non-section"));
		CAGE_TEST(ini->sectionsCount() == 1);
		CAGE_TEST(ini->section(0) == "section");
		CAGE_TEST(ini->itemExists("section", "item"));
		CAGE_TEST(!ini->itemExists("section", "non-item"));
		CAGE_TEST(!ini->itemExists("non-section", "item"));
		CAGE_TEST(ini->itemsCount("section") == 1);
		CAGE_TEST(ini->itemsCount("non-section") == 0);
		CAGE_TEST(ini->item("section", 0) == "item");
		ini.clear();
	}

	{
		CAGE_TESTCASE("save ini");
		Holder<Ini> ini = newIni();
		for (uint32 s = 3; s < 6; s++)
		{
			for (uint32 i = 2; i < 7; i++)
				ini->set(Stringizer() + s, Stringizer() + i, Stringizer() + s + i);
			ini->set(Stringizer() + s, "42", "42");
		}
		ini->exportFile("testdir/test.ini");
		CAGE_TEST(pathIsFile("testdir/test.ini"));
	}

	{
		CAGE_TESTCASE("load ini");
		Holder<Ini> ini = newIni();
		ini->importFile("testdir/test.ini");
		for (uint32 s = 3; s < 6; s++)
		{
			for (uint32 i = 2; i < 7; i++)
				CAGE_TEST(ini->get(Stringizer() + s, Stringizer() + i) == Stringizer() + s + i);
			CAGE_TEST(ini->get(Stringizer() + s, "42") == "42");
		}
		CAGE_TEST(ini->get("section", "item") == "");
	}

	{
		CAGE_TESTCASE("parse command line arguments (no positional arguments)");
		Holder<Ini> ini = newIni();
		const char *const cmd[] = {
			"appName",
			"--long",
			"haha",
			"-s",
			"-o"
		};
		ini->parseCmd(sizeof(cmd) / sizeof(char*), cmd);
		testVectors(vectorize(ini->sections()), { "long", "s", "o" });
		testVectors(vectorize(ini->values("long")), { "haha" });
		testVectors(vectorize(ini->values("s")), { "true" });
		testVectors(vectorize(ini->values("o")), { "true" });
	}

	{
		CAGE_TESTCASE("parse command line arguments (with positional arguments)");
		Holder<Ini> ini = newIni();
		const char *const cmd[] = {
			"appName",
			"pos1",
			"-abc",
			"very",
			"nice",
			"--long",
			"--",
			"pos2"
		};
		ini->parseCmd(sizeof(cmd) / sizeof(char*), cmd);
		testVectors(vectorize(ini->sections()), { "a", "b", "c", "long", "--" });
		testVectors(vectorize(ini->values("a")), { "true" });
		testVectors(vectorize(ini->values("b")), { "true" });
		testVectors(vectorize(ini->values("c")), { "very", "nice" });
		testVectors(vectorize(ini->values("long")), { "true" });
		testVectors(vectorize(ini->values("--")), { "pos1", "pos2" });
	}

	{
		CAGE_TESTCASE("unused variables");
		Holder<Ini> ini = newIni();
		ini->set("a", "1", "a1");
		ini->set("a", "2", "a2");
		ini->set("b", "1", "b1");
		CAGE_TEST(!ini->isUsed("a", "1"));
		CAGE_TEST(!ini->isUsed("a", "2"));
		CAGE_TEST(!ini->isUsed("b", "1"));
		{
			String s, t;
			CAGE_TEST(ini->anyUnused(s, t));
			CAGE_TEST(s == "a" || s == "b");
			CAGE_TEST(t == "1" || t == "2");
		}
		ini->getString("a", "1");
		ini->getString("a", "2");
		CAGE_TEST(ini->isUsed("a", "1"));
		CAGE_TEST(ini->isUsed("a", "2"));
		CAGE_TEST(!ini->isUsed("b", "1"));
		{
			String s, t;
			CAGE_TEST(ini->anyUnused(s, t));
			CAGE_TEST(s == "b" && t == "1");
		}
		ini->getString("b", "1");
		CAGE_TEST(ini->isUsed("a", "1"));
		CAGE_TEST(ini->isUsed("a", "2"));
		CAGE_TEST(ini->isUsed("b", "1"));
		{
			String s, t;
			CAGE_TEST(!ini->anyUnused(s, t));
		}
	}

	{
		CAGE_TESTCASE("parse command line arguments with helpers");
		Holder<Ini> ini = newIni();
		const char *const cmd[] = {
			"appName",
			"pos1",
			"-abc",
			"very",
			"nice",
			"-n",
			"42",
			"--ccc",
			"indeed",
			"--long",
			"--",
			"pos2"
		};
		ini->parseCmd(sizeof(cmd) / sizeof(char*), cmd);
		CAGE_TEST(ini->cmdBool('a', "aaa")); // required option
		CAGE_TEST(ini->cmdBool('a', "aaa", false)); // optional option
		CAGE_TEST(ini->cmdBool('a', "aaa", true));
		CAGE_TEST(ini->cmdBool('a', ""));
		CAGE_TEST(ini->cmdBool('a', "", false));
		{
			String s, i;
			CAGE_TEST(ini->anyUnused(s, i));
		}
		testVectors(vectorize(ini->cmdArray('a', "aaa")), { "true" });
		CAGE_TEST(ini->cmdBool('b', "bbb"));
		testVectors(vectorize(ini->cmdArray('c', "")), { "very", "nice" });
		testVectors(vectorize(ini->cmdArray('c', "ccc")), { "very", "nice", "indeed" });
		testVectors(vectorize(ini->cmdArray(0, "ccc")), { "indeed" });
		CAGE_TEST(ini->cmdBool('l', "long"));
		CAGE_TEST(ini->cmdBool(0, "long"));
		CAGE_TEST(ini->cmdUint32('n', "num") == 42);
		testVectors(vectorize(ini->cmdArray(0, "--")), { "pos1", "pos2" });
		CAGE_TEST(ini->cmdBool('d', "ddd", true));
		CAGE_TEST(!ini->cmdBool('d', "ddd", false));
		testVectors(vectorize(ini->cmdArray('d', "ddd")), {});
		CAGE_TEST_THROWN(ini->cmdBool('l', "")); // missing required option
		CAGE_TEST_THROWN(ini->cmdBool('c', "ccc")); // contains multiple values
		CAGE_TEST_THROWN(ini->cmdBool('d', "ddd")); // missing required option
		{
			String s, i;
			CAGE_TEST(!ini->anyUnused(s, i));
		}
	}
}
