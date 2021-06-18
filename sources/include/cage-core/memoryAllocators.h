#ifndef guard_memoryAllocators_h_1swf4glmnb9s
#define guard_memoryAllocators_h_1swf4glmnb9s

#include "core.h"

namespace cage
{
	// fastest memory allocator
	// individual deallocations are forbidden
	// always use flush to clear all allocations
	// allocations can have any sizes and alignments (within the block size)

	struct CAGE_CORE_API MemoryAllocatorLinearCreateConfig
	{
		uintPtr blockSize = 4 * 1024 * 1024;
	};

	CAGE_CORE_API Holder<MemoryArena> newMemoryAllocatorLinear(const MemoryAllocatorLinearCreateConfig &config);

	// linear allocator with individual deallocations
	// both individual deallocations (in any order) and flushing are available
	// blocks are reused in fifo order and only after fully cleared
	// allocations can have any sizes and alignments (within the block size)

	struct CAGE_CORE_API MemoryAllocatorStreamCreateConfig
	{
		uintPtr blockSize = 4 * 1024 * 1024;
	};

	CAGE_CORE_API Holder<MemoryArena> newMemoryAllocatorStream(const MemoryAllocatorStreamCreateConfig &config);

	// allocator facade for use in std containers

	template<class T>
	struct MemoryAllocatorStd
	{
		using value_type = T;

		MemoryAllocatorStd() : a(systemMemory())
		{}

		explicit MemoryAllocatorStd(const MemoryArena &arena) : a(arena)
		{}

		template<class TT>
		explicit MemoryAllocatorStd(const MemoryAllocatorStd<TT> &other) : a(other.a)
		{}

		T *allocate(uintPtr cnt)
		{
			return (T *)a.allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(T *ptr, uintPtr)
		{
			a.deallocate(ptr);
		}

	private:
		MemoryArena a;

		template<class TT>
		friend struct MemoryAllocatorStd;
	};
}

#endif // guard_memoryAllocators_h_1swf4glmnb9s
