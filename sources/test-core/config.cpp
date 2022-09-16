#include "main.h"

#include <cage-core/config.h>

namespace
{
	void printVariables()
	{
		CAGE_TESTCASE("traversing variables:");
		Holder<ConfigList> iter = newConfigList();
		while (iter->valid())
		{
			String res = iter->name();
			if (iter->type() != ConfigTypeEnum::Undefined)
				res += String(" = ") + iter->getString();
			res += String(" (") + iter->typeName() + ")";
			CAGE_LOG_CONTINUE(SeverityEnum::Note, "test", res);
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

	ConfigSint32 s32("test/b");
	s32 = 10;
	CAGE_TEST(configGetSint32("test/b") == 10);
	CAGE_TEST(s32 == 10);
	s32 = 20;
	CAGE_TEST(configGetSint32("test/b") == 20);
	CAGE_TEST(s32 == 20);

	ConfigBool b("test/b");
	CAGE_TEST(b); // b is still set to 20
	CAGE_TEST(configGetType("test/b") == ConfigTypeEnum::Sint32); // reading value does not change type
	CAGE_TEST(s32 == 20);
	b = false;
	CAGE_TEST(!b);
	CAGE_TEST(configGetType("test/b") == ConfigTypeEnum::Bool); // setting value changes type
	CAGE_TEST(s32 == 0); // the underlying value has changed

	ConfigBool b2_1("test2/b1");
	ConfigBool b2_2("test2/b2");
	ConfigBool b3_1("test3/b1");
	ConfigBool b3_2("test3/b2");
	b3_2 = true;

	ConfigSint32 c1("test/c"); // undefined
	CAGE_TEST(configGetType("test/c") == ConfigTypeEnum::Undefined);
	CAGE_TEST(configGetSint32("test/c", 5) == 5);
	CAGE_TEST(configGetSint32("test/c", 15) == 15); // getter does not set the value (even for config of undefined type)
	ConfigSint32 c2("test/c", 42); // sint32
	CAGE_TEST(configGetType("test/c") == ConfigTypeEnum::Sint32);
	CAGE_TEST(configGetSint32("test/c", 5) == 42);
	ConfigSint32 c3("test/c", 128); // sint32
	CAGE_TEST(configGetType("test/c") == ConfigTypeEnum::Sint32);
	CAGE_TEST(configGetSint32("test/c", 5) == 42);

	ConfigSint32 d("test/d"); // undefined
	CAGE_TEST(d == 0); // reading value from undefined config should return default value

	configSetSint32("test/empty", 42);
	CAGE_TEST(configGetType("test/empty") == ConfigTypeEnum::Sint32);
	configSetDynamic("test/empty", "");
	CAGE_TEST(configGetType("test/empty") == ConfigTypeEnum::String);

	configSetDynamic("test/dynamic", "42");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Uint32);
	CAGE_TEST(configGetUint32("test/dynamic") == 42);
	configSetDynamic("test/dynamic", "-42");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Sint32);
	CAGE_TEST(configGetSint32("test/dynamic") == -42);
	configSetDynamic("test/dynamic", "123456789000");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Uint64);
	CAGE_TEST(configGetUint64("test/dynamic") == 123456789000);
	configSetDynamic("test/dynamic", "-123456789000");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Sint64);
	CAGE_TEST(configGetSint64("test/dynamic") == -123456789000);
	configSetDynamic("test/dynamic", "trUE");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Bool);
	CAGE_TEST(configGetBool("test/dynamic") == true);
	configSetDynamic("test/dynamic", "False");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Bool);
	CAGE_TEST(configGetBool("test/dynamic") == false);
	configSetDynamic("test/dynamic", "3.5");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::Float || configGetType("test/dynamic") == ConfigTypeEnum::Double);
	CAGE_TEST(configGetFloat("test/dynamic") == 3.5 && configGetDouble("test/dynamic") == 3.5);
	configSetDynamic("test/dynamic", "Orangutan");
	CAGE_TEST(configGetType("test/dynamic") == ConfigTypeEnum::String);
	CAGE_TEST(configGetString("test/dynamic") == "Orangutan");

	printVariables();
}
