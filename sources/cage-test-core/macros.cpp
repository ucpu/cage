#include "main.h"

void testMacros()
{
	CAGE_TESTCASE("macros");

	{
		CAGE_TESTCASE("CAGE_NUM_ARGS");

		CAGE_TEST(CAGE_EVAL_SMALL(CAGE_NUM_ARGS()) == 0);
		CAGE_TEST(CAGE_EVAL_SMALL(CAGE_NUM_ARGS(a)) == 1);
		CAGE_TEST(CAGE_EVAL_SMALL(CAGE_NUM_ARGS(42, 13)) == 2);
		CAGE_TEST(CAGE_EVAL_SMALL(CAGE_NUM_ARGS(abc, def, ghi)) == 3);
		CAGE_TEST(CAGE_EVAL_SMALL(CAGE_NUM_ARGS(a, b, c, d, e, f, g, h, i)) == 9);
	}

	{
		CAGE_TESTCASE("CAGE_EXPAND_ARGS");

		uint32 tmp = 0;
#define ADD(X) tmp += X;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(ADD, 13, 17, 42));
#undef ADD
		CAGE_TEST(tmp == 13 + 17 + 42);
	}

	{
		CAGE_TESTCASE("CAGE_REPEAT");

		uint32 tmp = 0;
#define ADD(X) tmp += X;
		CAGE_EVAL_SMALL(CAGE_REPEAT(5, ADD));
#undef ADD
		CAGE_TEST(tmp == 1 + 2 + 3 + 4 + 5);
	}
}