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

	template<uintPtr Size> struct testStruct
	{
		int a;
		char dummy[Size - sizeof(int)];
		testStruct(int a = 0) : a(a) {}
		const bool operator < (const testStruct &other) const { return a < other.a; }
	};

#ifdef CAGE_SYSTEM_WINDOWS
#pragma pack(pop)
#endif

	template<uintPtr Limit, class T = void*> struct interceptMemoryArena
	{
		typedef T value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef uintPtr size_type;
		typedef sintPtr difference_type;

		template<class U> struct rebind
		{
			typedef interceptMemoryArena <Limit, U> other;
		};

		interceptMemoryArena()
		{}

		interceptMemoryArena(const interceptMemoryArena &other)
		{}

		template<class U> explicit interceptMemoryArena(const interceptMemoryArena<Limit, U> &other)
		{}

		pointer address(reference r) const
		{
			return &r;
		}

		const_pointer address(const_reference r) const
		{
			return &r;
		}

		pointer allocate(size_type cnt, void *hint = 0)
		{
			//CAGE_LOG(severityEnum::Info, "allocation", string() + "allocation, cnt: " + cnt + ", size: " + sizeof(T) + ", limit: " + Limit);
			if (cnt * sizeof(T) > Limit)
			{
				CAGE_LOG(severityEnum::Note, "exception", string() + "sizeof(T): " + sizeof(T));
				CAGE_LOG(severityEnum::Note, "exception", string() + "cnt: " + cnt);
				CAGE_LOG(severityEnum::Note, "exception", string() + "Limit: " + Limit);
				CAGE_THROW_CRITICAL(exception, "Insufficient memory allocator pool unit size");
			}
			return (pointer)detail::systemArena().allocate(cnt * sizeof(T));
		}

		void deallocate(pointer ptr, size_type cnt)
		{
			//CAGE_LOG(severityEnum::Info, "allocation", string() + "deallocation, cnt: " + cnt + ", size: " + sizeof(T) + ", limit: " + Limit);
			detail::systemArena().deallocate(ptr);
		}

		const size_type max_size() const noexcept
		{
			return 0;
		}

		void construct(pointer ptr, const T &t)
		{
			new (ptr) T(t);
		}

		void destroy(pointer p)
		{
			p->~T();
		}

		const bool operator == (const interceptMemoryArena &other) const
		{
			return true;
		}

		const bool operator != (const interceptMemoryArena &other) const
		{
			return false;
		}
	};

	template<uintPtr TestStructSize> struct memoryArenaStdTest
	{
		typedef testStruct<TestStructSize> ts;

		memoryArenaStdTest()
		{
			if (sizeof(ts) != TestStructSize)
			{
				CAGE_LOG(severityEnum::Warning, "test", string() + "memoryArenaStd tests skipped, requested size: " + TestStructSize + ", real size: " + sizeof(ts));
				return;
			}

			CAGE_TESTCASE(string() + "memoryArenaStd sizes, " + sizeof(ts));
			{
				CAGE_TESTCASE("list");
				std::list<ts, interceptMemoryArena<sizeof(templates::allocatorSizeList<ts>)>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont.push_back(ts(i));
			}
			{
				CAGE_TESTCASE("map");
				std::map<int, ts, std::less<int>, interceptMemoryArena<sizeof(templates::allocatorSizeMap<int, ts>)>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont[i] = ts(i);
			}
			{
				CAGE_TESTCASE("set");
				std::set<ts, std::less<ts>, interceptMemoryArena<sizeof(templates::allocatorSizeSet<ts>)>> cont;
				for (uint32 i = 0; i < 3; i++)
					cont.insert(ts(i));
			}

			CAGE_TESTCASE(string() + "memoryArenaStd with pool allocator, " + sizeof(ts));
			{
				CAGE_TESTCASE("list");
				memoryArenaFixed<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeList<ts>)>, memoryConcurrentPolicyNone> arena(1024 * 1024);
				std::list<ts, memoryArenaStd<>> cont((memoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont.push_back(ts(i));
			}
			{
				CAGE_TESTCASE("map");
				memoryArenaFixed<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeMap<int, ts>)>, memoryConcurrentPolicyNone> arena(1024 * 1024);
				std::map<int, ts, std::less<int>, memoryArenaStd<>> cont((std::less<int>(), memoryArena(&arena)));
				for (uint32 i = 0; i < 100; i++)
					cont[i] = ts(i);
			}
			{
				CAGE_TESTCASE("set");
				memoryArenaFixed<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeSet<ts>)>, memoryConcurrentPolicyNone> arena(1024 * 1024);
				std::set<ts, std::less<ts>, memoryArenaStd<>> cont((std::less<ts>(), memoryArena(&arena)));
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
