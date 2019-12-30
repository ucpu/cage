#include "main.h"
#include <cage-core/memory.h>
#include <cage-core/math.h>

#include <list>
#include <map>
#include <set>
#include <vector>

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
		Test
	{
		sint32 a;
		char dummy[Size - sizeof(sint32)];
		Test(sint32 a = 0) : a(a), dummy{} {}
		bool operator < (const Test &other) const { return a < other.a; }
	};
#ifdef CAGE_SYSTEM_WINDOWS
#pragma pack(pop)
#endif

	template<uintPtr Limit, class T>
	struct InterceptMemoryArena
	{
		typedef T value_type;

		InterceptMemoryArena()
		{}

		InterceptMemoryArena(const InterceptMemoryArena &other)
		{}

		template<class U>
		explicit InterceptMemoryArena(const InterceptMemoryArena<Limit, U> &other)
		{}

		template<class U>
		struct rebind
		{
			typedef InterceptMemoryArena<Limit, U> other;
		};

		T *allocate(uintPtr cnt)
		{
			if (cnt * sizeof(T) > Limit)
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "sizeof(T): " + sizeof(T));
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "cnt: " + cnt);
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "Limit: " + Limit);
				CAGE_THROW_CRITICAL(Exception, "insufficient atom size for pool allocator");
			}
			return (T*)detail::systemArena().allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(T *ptr, uintPtr) noexcept
		{
			detail::systemArena().deallocate(ptr);
		}

		bool operator == (const InterceptMemoryArena &other) const
		{
			return true;
		}

		bool operator != (const InterceptMemoryArena &other) const
		{
			return false;
		}
	};

	template<uintPtr TestStructSize>
	struct MemoryArenaStdTest
	{
		typedef Test<TestStructSize> ts;

		MemoryArenaStdTest()
		{
			if (sizeof(ts) != TestStructSize)
				CAGE_THROW_CRITICAL(Exception, "invalid padding is messing up with the test struct");

			CAGE_TESTCASE(stringizer() + "MemoryArenaStd sizes, " + sizeof(ts));
			{
				CAGE_TESTCASE("list");
				std::list<ts, InterceptMemoryArena<sizeof(templates::AllocatorSizeList<ts>), ts>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont.push_back(ts(i));
			}
			{
				CAGE_TESTCASE("map");
				std::map<int, ts, std::less<int>, InterceptMemoryArena<sizeof(templates::AllocatorSizeMap<int, ts>), std::pair<const int, ts>>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont[i] = ts(i);
			}
			{
				CAGE_TESTCASE("set");
				std::set<ts, std::less<ts>, InterceptMemoryArena<sizeof(templates::AllocatorSizeSet<ts>), ts>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont.insert(ts(i));
			}

			CAGE_TESTCASE(stringizer() + "MemoryArenaStd with pool allocator, " + sizeof(ts));
			{
				CAGE_TESTCASE("list");
				MemoryArenaFixed<MemoryAllocatorPolicyPool<sizeof(templates::AllocatorSizeList<ts>)>, MemoryConcurrentPolicyNone> arena(1024 * 1024);
				std::list<ts, MemoryArenaStd<ts>> cont((MemoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont.push_back(ts(i));
			}
			{
				CAGE_TESTCASE("map");
				MemoryArenaFixed<MemoryAllocatorPolicyPool<sizeof(templates::AllocatorSizeMap<int, ts>)>, MemoryConcurrentPolicyNone> arena(1024 * 1024);
				std::map<int, ts, std::less<int>, MemoryArenaStd<std::pair<const int, ts>>> cont((std::less<int>(), MemoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont[i] = ts(i);
			}
			{
				CAGE_TESTCASE("set");
				MemoryArenaFixed<MemoryAllocatorPolicyPool<sizeof(templates::AllocatorSizeSet<ts>)>, MemoryConcurrentPolicyNone> arena(1024 * 1024);
				std::set<ts, std::less<ts>, MemoryArenaStd<ts>> cont((std::less<ts>(), MemoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont.insert(ts(i));
			}
		}
	};
}

void testMemoryPools()
{
	MemoryArenaStdTest<6>();
	MemoryArenaStdTest<8>();
	MemoryArenaStdTest<10>();
	MemoryArenaStdTest<12>();
	MemoryArenaStdTest<14>();
	MemoryArenaStdTest<16>();
	MemoryArenaStdTest<18>();
	MemoryArenaStdTest<20>();
	MemoryArenaStdTest<123>();
	MemoryArenaStdTest<10000>(); // larger than page size

	{
		CAGE_TESTCASE("createHolder");
		MemoryArenaFixed<MemoryAllocatorPolicyPool<32>, MemoryConcurrentPolicyNone> pool(1024 * 1024);
		MemoryArena arena((MemoryArena(&pool)));
		std::vector<Holder<Test<6>>> vecs;
		vecs.reserve(100);
		for (uint32 i = 0; i < 100; i++)
			vecs.push_back(arena.createHolder<Test<6>>(i));
	}
}
