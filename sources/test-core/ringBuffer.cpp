#include <cage-core/math.h>
#include <cage-core/ringBuffer.h>

#include "main.h"

namespace
{
	sint32 itemsCounter = 0;

	struct Counter : private Immovable
	{
		Counter() { itemsCounter++; }
		~Counter() { itemsCounter--; }
	};
}

void testRingBuffer()
{
	CAGE_TESTCASE("ring buffer");

	{
		RingBuffer<Holder<Counter>> buffer;
		for (uint32 i = 0; i < 1000; i++)
		{
			if (buffer.size() > 10 && randomChance() < 0.5)
				buffer.pop_front();
			else
				buffer.push_back(systemMemory().createHolder<Counter>());
			CAGE_TEST(itemsCounter == buffer.size());
		}
	}
	CAGE_TEST(itemsCounter == 0);
}
