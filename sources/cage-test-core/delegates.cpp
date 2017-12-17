#include "main.h"

namespace
{
	class pokus
	{
	public:
		pokus(int value) : value(value) {};

		void fnc1() {};
		void fnc2() const {};
		void fnc3() {};
		void fnc3() const {};

		int fnci1(int a) { return a; };
		int fnci2(int a) const { return a; };
		int fnci3(int a) { return a; };
		int fnci3(int a) const { return a; };

		int value;
		int fncv() { return value; };
	};
}

void testDelegates()
{
	CAGE_TESTCASE("delegates");

	CAGE_LOG(severityEnum::Info, "test", string() + "delegate<void()> size: " + numeric_cast<uint32>(sizeof(delegate<void()>)));
	CAGE_LOG(severityEnum::Info, "test", string() + "delegate<int(int)> size: " + numeric_cast<uint32>(sizeof(delegate<int(int)>)));
	CAGE_LOG(severityEnum::Info, "test", string() + "delegate<void(void*, void*, int)> size: " + numeric_cast<uint32>(sizeof(delegate<void(void*, void*, int)>)));

	pokus vp((3));
	const pokus cp((5));

	delegate<void()> d1;
	d1.bind<pokus, &pokus::fnc1>(&vp);
	d1.bind<pokus, &pokus::fnc2>(&cp);
	//d1.bind<pokus, static_cast<void(pokus::*)()>(&pokus::fnc3)>(&vp);
	//d1.bind<pokus, static_cast<void(pokus::*)()const>(&pokus::fnc3)>(&cp);

	delegate<int(int)> d2;
	d2.bind<pokus, &pokus::fnci1>(&vp);
	d2.bind<pokus, &pokus::fnci2>(&cp);
	//d2.bind<pokus, static_cast<int(pokus::*)(int)>(&pokus::fnci3)>(&vp);
	//d2.bind<pokus, static_cast<int(pokus::*)(int)const>(&pokus::fnci3)>(&cp);
	CAGE_TEST(d2(5) == 5);

	delegate<int()> d3;
	CAGE_TEST_ASSERTED(d3());
	d3.bind<pokus, &pokus::fncv>(&vp);
	CAGE_TEST(d3() == 3);
}

