#include "main.h"
#include <cage-core/memoryAllocators.h>
#include <cage-core/math.h>

#include <vector>
#include <list>
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
					uint8 *p = (uint8 *)arena->allocate(s, uintPtr(1) << randomRange(2, 7));
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
			CAGE_TESTCASE("randomized allocations");
			MemoryAllocatorLinearCreateConfig cfg;
			cfg.blockSize = 4096; // put the allocator under a lot of pressure to test multiple blocks
			Holder<MemoryArena> arena = newMemoryAllocatorLinear(cfg);
			for (uint32 round = 0; round < 20; round++)
			{
				for (uint32 i = 0; i < 20; i++)
					arena->allocate(randomRange(1, 1000), uintPtr(1) << randomRange(2, 8));
				arena->flush();
			}
		}
	}

	void testStream()
	{
		CAGE_TESTCASE("stream allocator");

		{
			CAGE_TESTCASE("flush empty arena");
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			arena->flush();
		}

		{
			CAGE_TESTCASE("basics raw");
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
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
			CAGE_TESTCASE("basics structs");
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			CAGE_TEST(Test<16>::count == 0); // sanity check
			std::vector<Holder<Test<16>>> v;
			v.reserve(100);
			for (uint32 i = 0; i < 100; i++)
				v.push_back(arena->createHolder<Test<16>>());
			CAGE_TEST(Test<16>::count == 100);
			for (const auto &it : v)
				it->check();
			v.clear();
			CAGE_TEST(Test<16>::count == 0);
		}

		{
			CAGE_TESTCASE("large structs");
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			CAGE_TEST(Test<320>::count == 0); // sanity check
			std::vector<Holder<Test<320>>> v;
			v.reserve(100);
			for (uint32 i = 0; i < 100; i++)
				v.push_back(arena->createHolder<Test<320>>());
			CAGE_TEST(Test<320>::count == 100);
			for (const auto &it : v)
				it->check();
			v.clear();
			CAGE_TEST(Test<320>::count == 0);
		}

		{
			CAGE_TESTCASE("over-aligned structs");
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			CAGE_TEST(Test<320>::count == 0); // sanity check
			std::vector<Holder<AlignedTest<320, 16>>> v;
			v.reserve(100);
			for (uint32 i = 0; i < 100; i++)
				v.push_back(arena->createHolder<AlignedTest<320, 16>>());
			CAGE_TEST(Test<320>::count == 100);
			for (const auto &it : v)
				it->check();
			v.clear();
			CAGE_TEST(Test<320>::count == 0);
		}

		{
			CAGE_TESTCASE("random order deallocations");
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			CAGE_TEST(Test<16>::count == 0); // sanity check
			std::vector<Holder<Test<16>>> v;
			v.reserve(100);
			for (uint32 i = 0; i < 100; i++)
				v.push_back(arena->createHolder<Test<16>>());
			CAGE_TEST(Test<16>::count == 100);
			for (const auto &it : v)
				it->check();
			for (uint32 i = 0; i < 100; i++)
				v.erase(v.begin() + randomRange(uintPtr(0), v.size()));
			CAGE_TEST(Test<16>::count == 0);
		}

		{
			CAGE_TESTCASE("randomized allocations");
			MemoryAllocatorStreamCreateConfig cfg;
			cfg.blockSize = 4096; // put the allocator under a lot of pressure to test multiple blocks
			Holder<MemoryArena> arena = newMemoryAllocatorStream(cfg);
			std::vector<void *> v;
			v.reserve(1000);
			for (uint32 round = 0; round < 100; round++)
			{
				for (uint32 i = 0; i < 10; i++)
					v.push_back(arena->allocate(randomRange(1, 1000), uintPtr(1) << randomRange(2, 8)));
				for (uint32 i = 0; i < 8; i++)
				{
					const uint32 k = numeric_cast<uint32>(randomRange(uintPtr(0), v.size()));
					arena->deallocate(v[k]);
					v.erase(v.begin() + k);
				}
			}
			while (!v.empty())
			{
				const uint32 k = numeric_cast<uint32>(randomRange(uintPtr(0), v.size()));
				arena->deallocate(v[k]);
				v.erase(v.begin() + k);
			}
		}
	}

	void testStd()
	{
		CAGE_TESTCASE("std allocator");

		struct alignas(32) Elem
		{
			char data[64];
		};

		{
			CAGE_TESTCASE("vector");
			std::vector<Elem, MemoryAllocatorStd<Elem>> vec;
			for (uint32 i = 0; i < 100; i++)
				vec.emplace_back();
			for (const auto &it : vec)
				CAGE_TEST(((uintPtr)&it % alignof(Elem) == 0));
		}

		{
			CAGE_TESTCASE("list");
			std::list<Elem, MemoryAllocatorStd<Elem>> vec;
			for (uint32 i = 0; i < 100; i++)
				vec.emplace_back();
			for (const auto &it : vec)
				CAGE_TEST(((uintPtr)&it % alignof(Elem) == 0));
		}
	}
}

void testMemoryAllocators()
{
	CAGE_TESTCASE("memory allocators");

	testLinear();
	testStream();
	testStd();
}
