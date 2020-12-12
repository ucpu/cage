#include "main.h"
#include <cage-core/memoryAllocators.h>
#include <cage-core/math.h>

#include <vector>
#include <utility>

namespace
{
	void construct(uint8 *arr, const uint32 size)
	{
		for (uint32 i = 0; i < size; i++)
			arr[i] = i + 10;
	}

	void destruct(const uint8 *arr, const uint32 size)
	{
		for (uint32 i = 0; i < size; i++)
			CAGE_TEST(arr[i] == uint8(i + 10)); // do not use numeric_cast
	}

	template<uint32 Size>
	struct Test : private Immovable
	{
		Test()
		{
			count++;
			construct(arr, Size);
		}

		~Test()
		{
			check();
			count--;
		}

		void check() const
		{
			destruct(arr, Size);
		}

		static sint32 count;

	private:
		uint8 arr[Size];
	};

	template <uint32 Size, uintPtr Alignment>
	struct alignas(Alignment) AlignedTest : public Test<Size>
	{};

	template<uint32 Size>
	sint32 Test<Size>::count = 0;

	void testLinear()
	{
		CAGE_TESTCASE("linear allocator");

		{
			CAGE_TESTCASE("flush empty arena");
			Holder<MemoryArena> arena = newMemoryAllocatorLinear({});
			arena->flush();
		}

		{
			CAGE_TESTCASE("basics");
			Holder<MemoryArena> arena = newMemoryAllocatorLinear({});
			std::vector<uint8 *> v;
			v.reserve(100);
			for (uint32 i = 0; i < 100; i++)
			{
				uint8 *p = (uint8 *)arena->allocate(16, 16);
				construct(p, 16);
				v.push_back(p);
			}
			for (const uint8 *p : v)
				destruct(p, 16);
			arena->flush();
		}

		{
			CAGE_TESTCASE("multiple flushes");
			Holder<MemoryArena> arena = newMemoryAllocatorLinear({});
			std::vector<uint8 *> v;
			v.reserve(100);
			for (uint32 round = 0; round < 5; round++)
			{
				for (uint32 i = 0; i < 100; i++)
				{
					uint8 *p = (uint8 *)arena->allocate(16, 16);
					construct(p, 16);
					v.push_back(p);
				}
				for (const uint8 *p : v)
					destruct(p, 16);
				arena->flush();
				v.clear();
			}
		}

		{
			CAGE_TESTCASE("varying sizes");
			Holder<MemoryArena> arena = newMemoryAllocatorLinear({});
			std::vector<std::pair<uint8 *, uint32>> v;
			v.reserve(100);
			for (uint32 round = 0; round < 5; round++)
			{
				for (uint32 i = 0; i < 100; i++)
				{
					uint32 s = randomRange(1, 1000);
					uint8 *p = (uint8 *)arena->allocate(s, 16);
					construct(p, s);
					v.push_back({ p, s });
				}
				for (const auto &it : v)
					destruct(it.first, it.second);
				arena->flush();
				v.clear();
			}
		}

		{
			CAGE_TESTCASE("varying alignments");
			Holder<MemoryArena> arena = newMemoryAllocatorLinear({});
			std::vector<std::pair<uint8 *, uint32>> v;
			v.reserve(100);
			for (uint32 round = 0; round < 5; round++)
			{
				for (uint32 i = 0; i < 100; i++)
				{
					uint32 s = randomRange(1, 1000);
					uint8 *p = (uint8 *)arena->allocate(s, 1u << randomRange(2, 6));
					construct(p, s);
					v.push_back({ p, s });
				}
				for (const auto &it : v)
					destruct(it.first, it.second);
				arena->flush();
				v.clear();
			}
		}
	}
}

void testMemoryAllocators()
{
	CAGE_TESTCASE("memory allocators");

	testLinear();
}
