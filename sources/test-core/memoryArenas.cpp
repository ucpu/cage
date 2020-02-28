#include "main.h"
#include <cage-core/memory.h>
#include <cage-core/math.h>

#include <typeinfo>
#include <vector>

#define PrintTest CAGE_TESTCASE(string() + "arena: " + typeid(pool).name())

namespace
{
	void construct(void *pole, const uint32 size)
	{
		for (uint32 i = 0; i < size; i++)
			((uint8*)pole)[i] = i + 10;
	}

	void destruct(void *pole, const uint32 size)
	{
		for (uint32 i = 0; i < size; i++)
			CAGE_TEST(((uint8*)pole)[i] == (uint8)(i + 10)); // do not use numeric_cast
	}

	template<uint32 Size>
	struct Test
	{
		Test()
		{
			count++;
			construct(pole, Size);
		}
		~Test()
		{
			count--;
			check();
		}
		void check() const
		{
			destruct((void*)pole, Size);
		}
		static sint32 count;
	private:
		uint8 pole[Size];
	};

	template <uint32 Size, uintPtr Alignment>
	struct alignas(Alignment) AlignedTest : public Test<Size>
	{};

	template<uint32 Size>
	sint32 Test<Size>::count;

	template<template<class...> class ArenaPolicy,
		class BoundsPolicy, class TagPolicy, class TrackPolicy,
		uintPtr Alignment, uint32 AllocatorPolicy, class Traits>
	struct MemoryArenaTest {};

	template<template<class...> class ArenaPolicy,
		class BoundsPolicy, class TagPolicy, class TrackPolicy,
		uintPtr Alignment, class Traits>
	struct MemoryArenaTest<ArenaPolicy, BoundsPolicy, TagPolicy, TrackPolicy, Alignment, 0, Traits>
	{ // pool
		template<uintPtr Size>
		struct alignas(Alignment) AlignmentHelper
		{
			char data[Size];
		};

		MemoryArenaTest()
		{
			ArenaPolicy<MemoryAllocatorPolicyPool<templates::PoolAllocatorAtomSize<AlignmentHelper<Traits::AtomSize>>::result, BoundsPolicy, TagPolicy, TrackPolicy>, MemoryConcurrentPolicyNone> pool(Traits::MemoryLimit);
			PrintTest;
			MemoryArena a(&pool);
			std::vector<void*> alokace;
			alokace.reserve(Traits::ObjectsCount);
			for (uint32 i = 0; i < Traits::ObjectsCount * Traits::Rounds; i++)
			{
				if (alokace.size() == Traits::ObjectsCount || (alokace.size() > 0 && randomRange(0, 100) < 40))
				{
					uint32 index = randomRange((uint32)0, numeric_cast<uint32>(alokace.size()));
					CAGE_ASSERT(index < alokace.size(), index, alokace.size());
					void *tmp = alokace[index];
					uint16 sz = *(uint16*)tmp;
					destruct((char*)tmp + 2, sz - 2);
					a.deallocate(tmp);
					alokace.erase(alokace.begin() + index);
				}
				else
				{
					uint32 sz = randomRange((uint32)1, (uint32)(Traits::AtomSize - 1)) + (uint32)2;
					void *tmp = a.allocate(sz, Alignment);
					CAGE_TEST(uintPtr(tmp) % Alignment == 0, tmp, Alignment);
					(*(uint16*)tmp) = sz;
					construct((char*)tmp + 2, sz - 2);
					alokace.push_back(tmp);
				}
			}
			for (void *tmp : alokace)
			{
				uint16 sz = *(uint16*)tmp;
				destruct((char*)tmp + 2, sz - 2);
				a.deallocate(tmp);
			}
		}
	};

	template<template<class...> class ArenaPolicy,
		class BoundsPolicy, class TagPolicy, class TrackPolicy,
		uintPtr Alignment, class Traits>
	struct MemoryArenaTest<ArenaPolicy, BoundsPolicy, TagPolicy, TrackPolicy, Alignment, 1, Traits>
	{ // linear
		MemoryArenaTest()
		{
			ArenaPolicy<MemoryAllocatorPolicyLinear<BoundsPolicy, TagPolicy, TrackPolicy>, MemoryConcurrentPolicyNone> pool(Traits::MemoryLimit);
			PrintTest;
			MemoryArena a(&pool);
			typedef AlignedTest<Traits::ObjectSize, Alignment> ts;
			ts::count = 0;
			for (uint32 i = 0; i < Traits::ObjectsCount; i++)
			{
				void *tmp = a.createObject<ts>();
				CAGE_TEST(uintPtr(tmp) % Alignment == 0, tmp, Alignment);
			}
			CAGE_TEST(ts::count == Traits::ObjectsCount);
			a.flush();
		}
	};

	template<template<class...> class ArenaPolicy,
		class BoundsPolicy, class TagPolicy, class TrackPolicy,
		uintPtr Alignment, class Traits>
	struct MemoryArenaTest<ArenaPolicy, BoundsPolicy, TagPolicy, TrackPolicy, Alignment, 2, Traits>
	{ // nFrame
		MemoryArenaTest()
		{
			ArenaPolicy<MemoryAllocatorPolicyNFrame<Traits::Frames, BoundsPolicy, TagPolicy, TrackPolicy>, MemoryConcurrentPolicyNone> pool(Traits::MemoryLimit);
			PrintTest;
			MemoryArena a(&pool);
			typedef AlignedTest<Traits::ObjectSize, Alignment> ts;
			ts::count = 0;
			ts *last[Traits::Frames];
			for (uint32 i = 0; i < Traits::Frames; i++)
				last[i] = nullptr;
			for (uint32 round = 0; round < Traits::Rounds; round++)
			{
				for (uint32 i = 0; i < Traits::ObjectsCount; i++)
				{
					ts *l = a.createObject<ts>();
					CAGE_TEST(uintPtr(l) % Alignment == 0, l, Alignment);
					if (i == 0)
						last[round % Traits::Frames] = l;
				}
				for (uint32 i = 0; i < Traits::Frames; i++)
				{
					if (last[i])
						last[i]->check();
				}
				a.flush();
			}
			for (uint32 round = 0; round < Traits::Frames - 1; round++)
				a.flush();
		}
	};

	template<template<class...> class ArenaPolicy,
		class BoundsPolicy, class TagPolicy, class TrackPolicy,
		uintPtr Alignment, class Traits>
	struct MemoryArenaTest<ArenaPolicy, BoundsPolicy, TagPolicy, TrackPolicy, Alignment, 3, Traits>
	{ // queue
		MemoryArenaTest()
		{
			ArenaPolicy<MemoryAllocatorPolicyQueue<BoundsPolicy, TagPolicy, TrackPolicy>, MemoryConcurrentPolicyNone> pool(Traits::MemoryLimit);
			PrintTest;
			MemoryArena a(&pool);
			typedef AlignedTest<Traits::ObjectSize, Alignment> ts;
			ts::count = 0;
			ts *objects[Traits::ObjectsCount];
			uint32 allocations = 0;
			for (uint32 i = 0; i < Traits::ObjectsCount * Traits::Rounds; i++)
			{
				if (allocations == Traits::ObjectsCount || (allocations > 0 && randomRange(0, 100) < 40))
				{
					a.destroy<ts>(objects[0]);
					allocations--;
					for (uint32 j = 0; j < allocations; j++)
						objects[j] = objects[j + 1];
				}
				else
				{
					ts *tmp = a.createObject<ts>();
					CAGE_TEST(uintPtr(tmp) % Alignment == 0, tmp, Alignment);
					objects[allocations++] = tmp;
				}
				CAGE_TEST(allocations == ts::count, allocations, ts::count);
			}
			for (uint32 i = 0; i < allocations; i++)
				a.destroy<ts>(objects[i]);
			CAGE_TEST(ts::count == 0, ts::count);
		}
	};

	template<template<class...> class ArenaPolicy,
		class BoundsPolicy, class TagPolicy, class TrackPolicy,
		uintPtr Alignment, class Traits>
	struct MemoryArenaTest<ArenaPolicy, BoundsPolicy, TagPolicy, TrackPolicy, Alignment, 4, Traits>
	{ // stack
		MemoryArenaTest()
		{
			ArenaPolicy<MemoryAllocatorPolicyStack<BoundsPolicy, TagPolicy, TrackPolicy>, MemoryConcurrentPolicyNone> pool(Traits::MemoryLimit);
			PrintTest;
			MemoryArena a(&pool);
			typedef AlignedTest<Traits::ObjectSize, Alignment> ts;
			ts::count = 0;
			ts *objects[Traits::ObjectsCount];
			uint32 allocations = 0;
			for (uint32 i = 0; i < Traits::ObjectsCount * Traits::Rounds; i++)
			{
				if (allocations == Traits::ObjectsCount || (allocations > 0 && randomRange(0, 100) < 40))
					a.destroy<ts>(objects[--allocations]);
				else
				{
					ts *tmp = a.createObject<ts>();
					CAGE_TEST(uintPtr(tmp) % Alignment == 0, tmp, Alignment);
					objects[allocations++] = tmp;
				}
				CAGE_TEST(allocations == ts::count, allocations, ts::count);
			}
			for (uint32 i = 0, e = allocations; i < e; i++)
				a.destroy<ts>(objects[--allocations]);
			CAGE_TEST(ts::count == 0, ts::count);
		}
	};

	template<uintPtr ObjectSize_ = 42, uintPtr AtomSize_ = 42, uint32 Frames_ = 2, uint32 Rounds_ = 10, uint32 ObjectsCount_ = 2048, uintPtr MemoryLimit_ = 1024 * 1024 * 8>
	struct Traits
	{
		static const uintPtr ObjectSize = ObjectSize_;
		static const uintPtr AtomSize = AtomSize_;
		static const uint32 Frames = Frames_;
		static const uint32 Rounds = Rounds_;
		static const uint32 ObjectsCount = ObjectsCount_;
		static const uintPtr MemoryLimit = MemoryLimit_;
	};
}

void testMemoryArenas()
{
	CAGE_TESTCASE("memory arena");
	// pool
	{ MemoryArenaTest<MemoryArenaFixed, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 0, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 0, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicyNone, MemoryTagPolicyNone, MemoryTrackPolicyNone, 8, 0, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 32, 0, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicyNone, MemoryTagPolicyNone, MemoryTrackPolicyNone, 32, 0, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicyAdvanced, 8, 0, Traits<42, 42, 2, 2, 30>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 0, Traits<13>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 0, Traits<42, 2200, 2, 3>> a; }
	// linear
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 1, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 32, 1, Traits<>> a; }
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicyNone, MemoryTagPolicyNone, MemoryTrackPolicyNone, 8, 1, Traits<>> a; }
	// nFrames
	//{ MemoryArenaTest<MemoryArenaFixed, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 2, Traits<>> a; }
	//{ MemoryArenaTest<MemoryArenaFixed, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 32, 2, Traits<>> a; }
	//{ MemoryArenaTest<MemoryArenaFixed, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 32, 2, Traits<13, 42, 3>> a; }
	//{ MemoryArenaTest<MemoryArenaFixed, MemoryBoundsPolicyNone, MemoryTagPolicyNone, MemoryTrackPolicyNone, 8, 2, Traits<>> a; }
	// queue
	{ MemoryArenaTest<MemoryArenaFixed, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 3, Traits<>> a; }
	// stack
	{ MemoryArenaTest<MemoryArenaGrowing, MemoryBoundsPolicySimple, MemoryTagPolicySimple, MemoryTrackPolicySimple, 8, 4, Traits<>> a; }
}
