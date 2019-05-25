#include <cstdlib>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/timer.h>

#ifdef CAGE_DEBUG
#define INPUT_SIZE 100
#define CYCLES_COUNT 1000
#else
#define INPUT_SIZE 1000
#define CYCLES_COUNT 1000000
#endif

namespace
{
	void *allocated[INPUT_SIZE + 1];
	uintPtr allocSize;

	char tmpBuffer[512];

	void *write(void *ptr)
	{
		detail::memset(ptr, 1, allocSize);
		return ptr;
	}

	void *read(void *ptr)
	{
		detail::memcpy(tmpBuffer, ptr, allocSize);
		return ptr;
	}

	void *allocateSystem()
	{
		return write(::malloc(allocSize));
	}

	void deallocateSystem(void *ptr)
	{
		::free(read(ptr));
	}

	memoryArena arena;

	void *allocateMemory()
	{
		return write(arena.allocate(allocSize, sizeof(uintPtr)));
	}

	void deallocateMemory(void *ptr)
	{
		arena.deallocate(read(ptr));
	}

	template<void *Allocate(), void Deallocate(void *)>
	uint64 measure()
	{
		detail::memset(allocated, 1, (INPUT_SIZE + 1) * sizeof(void*));
		holder<timerClass> tmr = newTimer();
		uint32 allocations = 0;
		for (uint32 cycle = 0; cycle < CYCLES_COUNT; cycle++)
		{
			if (allocations == INPUT_SIZE || (allocations > 0 && randomRange(0, 100) < 30))
			{
				uint32 index = randomRange((uint32)0, allocations--);
				Deallocate(allocated[index]);
				allocated[index] = allocated[allocations];
			}
			else
			{
				allocated[allocations++] = Allocate();
			}
		}
		for (uint32 i = 0; i < allocations; i++)
			Deallocate(allocated[i]);
		return tmr->microsSinceStart();
	}

	template<class Concurrent, uintPtr AllocSize>
	void measureArena()
	{
		typedef memoryArenaGrowing<memoryAllocatorPolicyPool<AllocSize>, Concurrent> pool;
		pool a((INPUT_SIZE + 5) * (AllocSize + sizeof(uintPtr) * 3));
		arena = memoryArena(&a);

		uint64 system = measure<&allocateSystem, &deallocateSystem>();
		uint64 memory = measure<&allocateMemory, &deallocateMemory>();
		CAGE_LOG(severityEnum::Info, "performance", string() + "timing: " + system + "\t" + memory + "\t" + (memory < system ? "better" : "worse") + "\t" + ((float)memory / (float)system));
	}

	template<uintPtr AllocSize>
	void measureAllocSize(uintPtr realAllocs)
	{
		CAGE_LOG(severityEnum::Info, "performance", string() + "Atom " + AllocSize + ", allocations " + realAllocs + " bytes");
		allocSize = realAllocs;
		CAGE_LOG(severityEnum::Info, "performance", "no concurrency");
		measureArena<memoryConcurrentPolicyNone, AllocSize>();
		CAGE_LOG(severityEnum::Info, "performance", "mutex arena");
		measureArena<memoryConcurrentPolicyMutex, AllocSize>();
	}
}

void testMemoryPerformance()
{
	CAGE_TESTCASE("test pool memory arena performance");
	measureAllocSize<8>(8);
	measureAllocSize<32>(20);
	measureAllocSize<32>(30);
	measureAllocSize<512>(300);
	measureAllocSize<512>(500);
}
