#include "main.h"
#include <cage-core/endianness.h>

#include <cmath> // std::isnan

void testNumericTypes()
{
	CAGE_TESTCASE("numeric types");

	{
		CAGE_TESTCASE("numeric_cast");
		// shrink signed
		CAGE_TEST(numeric_cast<sint8>((sint32)5) == 5);
		CAGE_TEST(numeric_cast<sint8>((sint32)-5) == -5);
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((sint32)300));
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((sint32)-300));
		// enlarge signed
		CAGE_TEST(numeric_cast<sint32>((sint8)5) == 5);
		CAGE_TEST(numeric_cast<sint32>((sint8)-5) == -5);
		// shrink unsigned
		CAGE_TEST(numeric_cast<uint8>((uint32)200) == 200);
		CAGE_TEST_ASSERTED(numeric_cast<uint8>((uint32)300));
		// enlarge unsigned
		CAGE_TEST(numeric_cast<uint32>((uint8)200) == 200);
		// unsigned -> signed
		CAGE_TEST(numeric_cast<sint32>((uint32)4) == 4);
		CAGE_TEST(numeric_cast<sint8>((uint8)4) == 4);
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((uint8)200));
		// signed -> unsigned
		CAGE_TEST(numeric_cast<uint32>((sint32)4) == 4);
		CAGE_TEST_ASSERTED(numeric_cast<uint32>((sint32)-4));
		// float -> double
		CAGE_TEST(numeric_cast<double>((float)4) == 4);
		CAGE_TEST(std::isnan(numeric_cast<double>(std::numeric_limits<float>::quiet_NaN())));
		// double -> float
		CAGE_TEST(numeric_cast<float>((double)4) == 4);
		CAGE_TEST(numeric_cast<float>((double)1e-50) >= 0); // numeric_cast does not detect underflows with floating point numbers
		CAGE_TEST(numeric_cast<float>((double)-1e-50) <= 0);
		{
			double a = 1e50; // avoid warning about overflow in constant arithmetic
			double b = -1e50;
			CAGE_TEST(numeric_cast<float>(a) > 1e20); // numeric_cast allows conversions with infinites
			CAGE_TEST(numeric_cast<float>(b) < -1e20);
		}
		CAGE_TEST(std::isnan(numeric_cast<float>(std::numeric_limits<double>::quiet_NaN())));
		// float -> signed
		CAGE_TEST(numeric_cast<sint8>((float)5.1) == 5);
		CAGE_TEST(numeric_cast<sint8>((float)-5.1) == -5);
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((float)200));
		CAGE_TEST_ASSERTED(numeric_cast<sint8>((float)-200));
		// float -> unsigned
		CAGE_TEST(numeric_cast<uint8>((float)5.1) == 5);
		CAGE_TEST(numeric_cast<uint8>((float)200) == 200);
		CAGE_TEST_ASSERTED(numeric_cast<uint8>((float)-5));
		CAGE_TEST_ASSERTED(numeric_cast<uint8>((float)500));
		// integer -> float
		CAGE_TEST(numeric_cast<float>((sint32)42) == 42);
		CAGE_TEST(numeric_cast<float>((sint32)-42) == -42);
		CAGE_TEST(numeric_cast<float>((uint32)42) == 42);
	}

	{
		CAGE_TESTCASE("endianity");
		CAGE_TEST(endianness::change((uint16)0x1234) == (uint16)0x3412);
		CAGE_TEST(endianness::change((uint32)0x12345678) == (uint32)0x78563412);
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
