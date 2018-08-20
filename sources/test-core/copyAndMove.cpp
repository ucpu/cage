#include <vector>

#include "main.h"

#include <cage-core/utility/memoryBuffer.h>

void testCopyAndMove()
{
	CAGE_TESTCASE("copy and move semantics");

	{
		CAGE_TESTCASE("vector of strings");
		std::vector<string> vec;
		vec.emplace_back("hi");
	}

	{
		CAGE_TESTCASE("vector of memory buffers");
		std::vector<memoryBuffer> vec;
		vec.emplace_back(memoryBuffer());
	}
}
