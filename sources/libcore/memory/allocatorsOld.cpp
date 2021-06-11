#include <cage-core/memoryAllocatorsOld.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include <map>

namespace cage
{
	MemoryConcurrentPolicyMutex::MemoryConcurrentPolicyMutex()
	{
		mutex = newMutex();
	}

	void MemoryConcurrentPolicyMutex::lock()
	{
		mutex->lock();
	}

	void MemoryConcurrentPolicyMutex::unlock()
	{
		mutex->unlock();
	}

	MemoryTrackPolicySimple::~MemoryTrackPolicySimple()
	{
		if (count > 0)
		{
			CAGE_LOG(SeverityEnum::Critical, "memory", "memory leak detected");
			detail::terminate();
		}
	}

	namespace
	{
		struct Allocation
		{
			uint64 time;
			uint64 thread;
			uintPtr size;
		};

		struct MemoryTrackPolicyAdvancedImpl
		{
			MemoryTrackPolicyAdvancedImpl()
			{}

			~MemoryTrackPolicyAdvancedImpl()
			{
				if (allocations.size() == 0)
					return;
				CAGE_LOG(SeverityEnum::Critical, "memory", "memory leak report");
				reportAllocatins();
				detail::terminate();
			}

			void set(void *ptr, uintPtr size)
			{
				CAGE_ASSERT(allocations.find(ptr) == allocations.end());
				Allocation a;
				a.size = size;
				a.thread = currentThreadId();
				a.time = CAGE_LOG(SeverityEnum::Info, "memory", stringizer() + "allocation at " + ptr + " of size " + size);
				allocations[ptr] = a;
			}

			void check(void *ptr)
			{
				CAGE_ASSERT(allocations.find(ptr) != allocations.end());
				allocations.erase(ptr);
				CAGE_LOG(SeverityEnum::Info, "memory", stringizer() + "deallocation at " + ptr);
			}

			void flush()
			{
				CAGE_LOG(SeverityEnum::Info, "memory", "flush");
				reportAllocatins();
				allocations.clear();
			}

			void reportAllocatins() const
			{
				for (std::map <void*, Allocation>::const_iterator i = allocations.begin(), e = allocations.end(); i != e; i++)
					CAGE_LOG_CONTINUE(SeverityEnum::Note, "memory", stringizer() + "memory at " + i->first + " of size " + i->second.size + " allocated in thread " + i->second.thread + " at time " + i->second.time);
			}

			std::map <void*, Allocation> allocations;
		};
	}

	MemoryTrackPolicyAdvanced::MemoryTrackPolicyAdvanced()
	{
		data = new MemoryTrackPolicyAdvancedImpl();
	}

	MemoryTrackPolicyAdvanced::~MemoryTrackPolicyAdvanced()
	{
		delete (MemoryTrackPolicyAdvancedImpl*)data;
	}

	void MemoryTrackPolicyAdvanced::set(void *ptr, uintPtr size)
	{
		((MemoryTrackPolicyAdvancedImpl*)data)->set(ptr, size);
	}

	void MemoryTrackPolicyAdvanced::check(void *ptr)
	{
		((MemoryTrackPolicyAdvancedImpl*)data)->check(ptr);
	}

	void MemoryTrackPolicyAdvanced::flush()
	{
		((MemoryTrackPolicyAdvancedImpl*)data)->flush();
	}
}
