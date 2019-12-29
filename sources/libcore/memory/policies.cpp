#include <map>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>

namespace cage
{
	memoryConcurrentPolicyMutex::memoryConcurrentPolicyMutex()
	{
		mutex = newSyncMutex().cast<void>();
	}

	void memoryConcurrentPolicyMutex::lock()
	{
		((Mutex*)mutex.get())->lock();
	}

	void memoryConcurrentPolicyMutex::unlock()
	{
		((Mutex*)mutex.get())->unlock();
	}

	memoryTrackPolicySimple::~memoryTrackPolicySimple()
	{
		if (count > 0)
		{
			CAGE_LOG(SeverityEnum::Critical, "memory", "memory leak detected");
			detail::terminate();
		}
	}

	namespace
	{
		struct allocation
		{
			uint64 time;
			uint64 thread;
			uintPtr size;
		};

		struct advancedTrackPolicyData
		{
			advancedTrackPolicyData()
			{}

			~advancedTrackPolicyData()
			{
				if (allocations.size() == 0)
					return;
				CAGE_LOG(SeverityEnum::Critical, "memory", "memory leak report");
				reportAllocatins();
				detail::terminate();
			}

			void set(void *ptr, uintPtr size)
			{
				CAGE_ASSERT(allocations.find(ptr) == allocations.end(), "duplicate allocation at same address");
				allocation a;
				a.size = size;
				a.thread = threadId();
				a.time = CAGE_LOG(SeverityEnum::Info, "memory", stringizer() + "allocation at " + ptr + " of size " + size);
				allocations[ptr] = a;
			}

			void check(void *ptr)
			{
				CAGE_ASSERT(allocations.find(ptr) != allocations.end(), "deallocation at unknown address");
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
				for (std::map <void*, allocation>::const_iterator i = allocations.begin(), e = allocations.end(); i != e; i++)
					CAGE_LOG_CONTINUE(SeverityEnum::Note, "memory", stringizer() + "memory at " + i->first + " of size " + i->second.size + " allocated in thread " + i->second.thread + " at time " + i->second.time);
			}

			std::map <void*, allocation> allocations;
		};
	}

	memoryTrackPolicyAdvanced::memoryTrackPolicyAdvanced()
	{
		data = new advancedTrackPolicyData();
	}

	memoryTrackPolicyAdvanced::~memoryTrackPolicyAdvanced()
	{
		delete (advancedTrackPolicyData*)data;
	}

	void memoryTrackPolicyAdvanced::set(void *ptr, uintPtr size)
	{
		((advancedTrackPolicyData*)data)->set(ptr, size);
	}

	void memoryTrackPolicyAdvanced::check(void *ptr)
	{
		((advancedTrackPolicyData*)data)->check(ptr);
	}

	void memoryTrackPolicyAdvanced::flush()
	{
		((advancedTrackPolicyData*)data)->flush();
	}
}
