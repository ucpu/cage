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
}

#endif // guard_memoryAllocators_h_1swf4glmnb9s
