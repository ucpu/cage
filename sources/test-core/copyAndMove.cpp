#include "main.h"

#include <cage-core/memoryBuffer.h>

#include <vector>

void testCopyAndMove()
{
	CAGE_TESTCASE("copy and move semantics");

	{
		CAGE_TESTCASE("vector of strings");
		std::vector<string> vec;
		vec.emplace_back("hi");
		vec.push_back("hello");
		std::vector<string> vec2 = templates::move(vec);
	}

	{
		CAGE_TESTCASE("vector of memory buffers");
		std::vector<MemoryBuffer> vec;
		vec.emplace_back(MemoryBuffer());
		vec.push_back(MemoryBuffer());
		std::vector<MemoryBuffer> vec2 = templates::move(vec);
	}
}
