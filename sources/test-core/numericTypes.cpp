#include "main.h"

void testNumericTypes()
{
	CAGE_TESTCASE("numeric types");

	{
		CAGE_TESTCASE("numeric_cast");
		// condense signed
		CAGE_TEST(numeric_cast<sint8>((sint32)5) == 5);
		CAGE_TEST(numeric_cast<sint8>((sint32)-5) == -5);
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((sint32)300));
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((sint32)-300));
		// enlarge signed
		CAGE_TEST(numeric_cast<sint32>((sint8)5) == 5);
		CAGE_TEST(numeric_cast<sint32>((sint8)-5) == -5);
		// condense unsigned
		CAGE_TEST(numeric_cast<uint8>((uint32)200) == 200);
		CAGE_TEST_ASSERTED(numeric_cast<uint8>((uint32)300));
		// enlarge unsigned
		CAGE_TEST(numeric_cast<uint32>((uint8)200) == 200);
		// unsigned -> signed
		CAGE_TEST(numeric_cast<sint32>((uint32)4) == 4);
		// signed -> unsigned
		CAGE_TEST(numeric_cast<uint32>((sint32)4) == 4);
		CAGE_TEST_ASSERTED(numeric_cast<uint32>((sint32)-4));
	}

	{
		CAGE_TESTCASE("endianity");
		CAGE_TEST(privat::endianness::change((uint16)0x1234) == (uint16)0x3412);
		CAGE_TEST(privat::endianness::change((uint32)0x12345678) == (uint32)0x78563412);
	}

	{
		CAGE_TESTCASE("maxStruct");
		uint32 k = m;
		CAGE_TEST(k == m);
		CAGE_TEST((uint8)-1 == m);
		CAGE_TEST((uint16)-1 == m);
		CAGE_TEST((uint32)-1 == m);
		CAGE_TEST((uint64)-1 == m);
		CAGE_TEST(!((uint8)5 == m));
		CAGE_TEST(!((uint16)13 == m));
		CAGE_TEST(!((uint32)42 == m));
		CAGE_TEST(!((uint64)0 == m));
		CAGE_TEST((sint8)127 == m);
		CAGE_TEST((sint16)32767 == m);
	}
}
