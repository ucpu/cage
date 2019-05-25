#include "main.h"
#include <cage-core/filesystem.h>
#include <cage-core/ini.h>

#include <vector>
#include <algorithm>

namespace
{
	void testVectors(std::vector<string> a, std::vector<string> b)
	{
		CAGE_TEST(a.size() == b.size());
		std::sort(a.begin(), a.end());
		std::sort(b.begin(), b.end());
		for (uint32 i = 0; i < a.size(); i++)
			CAGE_TEST(a[i] == b[i]);
	}

	std::vector<string> vectorize(const holder<pointerRange<string>> &r)
	{
		return std::vector<string>(r->begin(), r->end());
	}
}

void testIni()
{
	CAGE_TESTCASE("ini");

	{
		CAGE_TESTCASE("basic ini");
		holder<iniClass> ini = newIni(); // 1 MB memory
		ini->setString("section", "item", "value");
		CAGE_TEST(ini->getString("section", "item", "default") == "value");
		CAGE_TEST(ini->getString("section", "non-item", "default") == "default");
		CAGE_TEST(ini->sectionExists("section"));
		CAGE_TEST(!ini->sectionExists("non-section"));
		CAGE_TEST(ini->sectionsCount() == 1);
		CAGE_TEST(ini->section(0) == "section");
		CAGE_TEST(ini->itemExists("section", "item"));
		CAGE_TEST(!ini->itemExists("section", "non-item"));
		CAGE_TEST(ini->itemsCount("section") == 1);
		CAGE_TEST(ini->item("section", 0) == "item");
		ini.clear();
	}

	{
		CAGE_TESTCASE("save ini");
		holder<iniClass> ini = newIni(); // 1 MB memory
		for (uint32 s = 3; s < 6; s++)
			for (uint32 i = 2; i < 7; i++)
				ini->set(s, i, s + i);
		ini->save("testdir/test.ini");
		CAGE_TEST(pathIsFile("testdir/test.ini"));
	}

	{
		CAGE_TESTCASE("load ini");
		holder<iniClass> ini = newIni(); // 1 MB memory
		ini->load("testdir/test.ini");
		for (uint32 s = 3; s < 6; s++)
			for (uint32 i = 2; i < 7; i++)
				CAGE_TEST(ini->get(s, i) == string(s + i));
		CAGE_TEST(ini->get("section", "item") == "");
	}

	{
		CAGE_TESTCASE("parse command line arguments (no positional arguments)");
		holder<iniClass> ini = newIni();
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
		holder<iniClass> ini = newIni();
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
}
