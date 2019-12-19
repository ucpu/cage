#include "main.h"
#include <cage-core/config.h>

namespace
{
	void printVariables()
	{
		CAGE_TESTCASE("traversing variables:");
		holder<configList> iter = newConfigList();
		while (iter->valid())
		{
			string res = iter->name() + ": " + iter->typeName();
			if (iter->type() != configTypeEnum::Undefined)
				res += string(" = ") + iter->getString();
			CAGE_LOG_CONTINUE(severityEnum::Note, "test", res);
			iter->next();
		}
	}
}

void testConfig()
{
	CAGE_TESTCASE("config");

	configSetString("test/ahoj", "ahoj");
	CAGE_TEST(configGetString("test/ahoj") == "ahoj");
	configSetString("test/ahoj", "nazdar");
	CAGE_TEST(configGetString("test/ahoj") == "nazdar");
	configSetSint32("test/ahoj", 10);
	CAGE_TEST(configGetBool("test/ahoj") == true);
	CAGE_TEST(configGetSint32("test/ahoj") == 10);
	CAGE_TEST(configGetSint64("test/ahoj") == 10);
	CAGE_TEST(configGetFloat("test/ahoj") == 10);
	CAGE_TEST(configGetString("test/ahoj") == "10");

	configSint32 s32("test/b");
	s32 = 10;
	CAGE_TEST(configGetSint32("test/b") == 10);
	CAGE_TEST(s32 == 10);
	s32 = 20;
	CAGE_TEST(configGetSint32("test/b") == 20);
	CAGE_TEST(s32 == 20);

	configBool b("test/b");
	CAGE_TEST(b); // b is still set to 20
	b = false;
	CAGE_TEST(!b);

	configBool b2_1("test2/b1");
	configBool b2_2("test2/b2");
	configBool b3_1("test3/b1");
	configBool b3_2("test3/b2");
	b3_2 = true;

	configSint32 c1("test/c"); // undefined
	CAGE_TEST(configGetType("test/c") == configTypeEnum::Undefined);
	CAGE_TEST(configGetSint32("test/c", 5) == 5);
	CAGE_TEST(configGetSint32("test/c", 15) == 15); // getter does not set the value (even for config of undefined type)
	configSint32 c2("test/c", 42); // sint32
	CAGE_TEST(configGetType("test/c") == configTypeEnum::Sint32);
	CAGE_TEST(configGetSint32("test/c", 5) == 42);
	configSint32 c3("test/c", 128); // sint32
	CAGE_TEST(configGetType("test/c") == configTypeEnum::Sint32);
	CAGE_TEST(configGetSint32("test/c", 5) == 42);

	configSint32 d("test/d"); // undefined
	CAGE_TEST(d == 0); // reading value from undefined config should return default value

	printVariables();
}
