#include "main.h"
#include <cage-core/filesystem.h>
#include <cage-core/utility/ini.h>

void testIni()
{
	CAGE_TESTCASE("ini");

	{
		CAGE_TESTCASE("basic ini");
		holder<iniClass> ini = newIni();
		ini->setString("section", "item", "value");
		CAGE_TEST(ini->getString("section", "item", "default") == "value");
		CAGE_TEST(ini->getString("section", "non-item", "default") == "default");
		CAGE_TEST(ini->sectionExists("section"));
		CAGE_TEST(!ini->sectionExists("non-section"));
		CAGE_TEST(ini->sectionCount() == 1);
		CAGE_TEST(ini->section(0) == "section");
		CAGE_TEST(ini->itemExists("section", "item"));
		CAGE_TEST(!ini->itemExists("section", "non-item"));
		CAGE_TEST(ini->itemCount("section") == 1);
		CAGE_TEST(ini->item("section", 0) == "item");
		ini.clear();
	}

	{
		CAGE_TESTCASE("save ini");
		holder<iniClass> ini = newIni();
		for (uint32 s = 3; s < 6; s++)
			for (uint32 i = 2; i < 7; i++)
				ini->set(s, i, s + i);
		ini->save("testdir/test.ini");
		CAGE_TEST(newFilesystem()->exists("testdir/test.ini"));
	}

	{
		CAGE_TESTCASE("load ini");
		holder<iniClass> ini = newIni();
		ini->load("testdir/test.ini");
		for (uint32 s = 3; s < 6; s++)
			for (uint32 i = 2; i < 7; i++)
				CAGE_TEST(ini->get(s, i) == string(s + i));
		CAGE_TEST(ini->get("section", "item") == "");
	}
}