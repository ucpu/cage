#include <vector>

#include "main.h"

void testTemplates()
{
	CAGE_TESTCASE("templates");

	{
		CAGE_TESTCASE("pointer range");
		std::vector<uint32> numbers;
		numbers.push_back(5);
		numbers.push_back(42);
		numbers.push_back(13);
		uint32 sum = 0;
		for (auto it : pointerRange<uint32>(numbers))
			sum += it;
		CAGE_TEST(sum == 5 + 42 + 13);
	}
}
