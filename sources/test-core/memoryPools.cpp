#include <list>
#include <map>
#include <set>
#include <vector>

#include "main.h"
#include <cage-core/memory.h>
#include <cage-core/math.h>

namespace
{
#ifdef CAGE_SYSTEM_WINDOWS
#pragma pack(push,1)
#endif
	template<uintPtr Size>
	struct
#ifndef CAGE_SYSTEM_WINDOWS
		__attribute__((__packed__))
#endif // !CAGE_SYSTEM_WINDOWS
		testStruct
	{
		sint32 a;
		char dummy[Size - sizeof(sint32)];
		testStruct(sint32 a = 0) : a(a), dummy{} {}
		bool operator < (const testStruct &other) const { return a < other.a; }
	};
#ifdef CAGE_SYSTEM_WINDOWS
#pragma pack(pop)
#endif

	template<uintPtr Limit, class T>
	struct interceptMemoryArena
	{
		typedef T value_type;

		interceptMemoryArena()
		{}

		interceptMemoryArena(const interceptMemoryArena &other)
		{}

		template<class U>
		explicit interceptMemoryArena(const interceptMemoryArena<Limit, U> &other)
		{}

		template<class U>
		struct rebind
		{
			typedef interceptMemoryArena<Limit, U> other;
		};

		T *allocate(uintPtr cnt)
		{
			if (cnt * sizeof(T) > Limit)
			{
				CAGE_LOG(severityEnum::Note, "exception", stringizer() + "sizeof(T): " + sizeof(T));
				CAGE_LOG(severityEnum::Note, "exception", stringizer() + "cnt: " + cnt);
				CAGE_LOG(severityEnum::Note, "exception", stringizer() + "Limit: " + Limit);
				CAGE_THROW_CRITICAL(exception, "insufficient atom size for pool allocator");
			}
			return (T*)detail::systemArena().allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(T *ptr, uintPtr) noexcept
		{
			detail::systemArena().deallocate(ptr);
		}

		bool operator == (const interceptMemoryArena &other) const
		{
			return true;
		}

		bool operator != (const interceptMemoryArena &other) const
		{
			return false;
		}
	};

	template<uintPtr TestStructSize>
	struct memoryArenaStdTest
	{
		typedef testStruct<TestStructSize> ts;

		memoryArenaStdTest()
		{
			if (sizeof(ts) != TestStructSize)
				CAGE_THROW_CRITICAL(exception, "invalid padding is messing up with the test struct");

			CAGE_TESTCASE(stringizer() + "memoryArenaStd sizes, " + sizeof(ts));
			{
				CAGE_TESTCASE("list");
				std::list<ts, interceptMemoryArena<sizeof(templates::allocatorSizeList<ts>), ts>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont.push_back(ts(i));
			}
			{
				CAGE_TESTCASE("map");
				std::map<int, ts, std::less<int>, interceptMemoryArena<sizeof(templates::allocatorSizeMap<int, ts>), std::pair<const int, ts>>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont[i] = ts(i);
			}
			{
				CAGE_TESTCASE("set");
				std::set<ts, std::less<ts>, interceptMemoryArena<sizeof(templates::allocatorSizeSet<ts>), ts>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont.insert(ts(i));
			}

			CAGE_TESTCASE(stringizer() + "memoryArenaStd with pool allocator, " + sizeof(ts));
			{
				CAGE_TESTCASE("list");
				memoryArenaFixed<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeList<ts>)>, memoryConcurrentPolicyNone> arena(1024 * 1024);
				std::list<ts, memoryArenaStd<ts>> cont((memoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont.push_back(ts(i));
			}
			{
				CAGE_TESTCASE("map");
				memoryArenaFixed<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeMap<int, ts>)>, memoryConcurrentPolicyNone> arena(1024 * 1024);
				std::map<int, ts, std::less<int>, memoryArenaStd<std::pair<const int, ts>>> cont((std::less<int>(), memoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont[i] = ts(i);
			}
			{
				CAGE_TESTCASE("set");
				memoryArenaFixed<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeSet<ts>)>, memoryConcurrentPolicyNone> arena(1024 * 1024);
				std::set<ts, std::less<ts>, memoryArenaStd<ts>> cont((std::less<ts>(), memoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont.insert(ts(i));
			}
		}
	};
}

void testMemoryPools()
{
	memoryArenaStdTest<6>();
	memoryArenaStdTest<8>();
	memoryArenaStdTest<10>();
	memoryArenaStdTest<12>();
	memoryArenaStdTest<14>();
	memoryArenaStdTest<16>();
	memoryArenaStdTest<18>();
	memoryArenaStdTest<20>();
	memoryArenaStdTest<123>();
	memoryArenaStdTest<10000>(); // larger than page size

	{
		CAGE_TESTCASE("createHolder");
		memoryArenaFixed<memoryAllocatorPolicyPool<32>, memoryConcurrentPolicyNone> pool(1024 * 1024);
		memoryArena arena((memoryArena(&pool)));
		std::vector<holder<testStruct<6>>> vecs;
		vecs.reserve(100);
		for (uint32 i = 0; i < 100; i++)
			vecs.push_back(arena.createHolder<testStruct<6>>(i));
	}
}
