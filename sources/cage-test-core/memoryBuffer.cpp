#include "main.h"
#include <cage-core/utility/memoryBuffer.h>

void testMemoryBuffers()
{
	CAGE_TESTCASE("memory buffers");

	{
		CAGE_TESTCASE("compression");
		memoryBuffer b1(1000);
		for (uintPtr i = 0, e = b1.size(); i < e; i++)
			((uint8*)b1.data())[i] = (uint8)i;
		memoryBuffer b2 = detail::compress(b1);
		CAGE_TEST(b2.size() < b1.size());
		memoryBuffer b3 = detail::decompress(b2, 2000);
		CAGE_TEST(b3.size() == b1.size());
		CAGE_TEST(detail::memcmp(b3.data(), b1.data(), b1.size()) == 0);
	}
}
