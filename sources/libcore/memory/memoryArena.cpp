#include <cage-core/memoryArena.h>
#include <cage-core/memoryUtils.h> // isPowerOf2

#include <atomic>
#include <cstdlib>

namespace cage
{
	void *MemoryArena::allocate(uintPtr size, uintPtr alignment)
	{
		void *res = stub->alloc(inst, size, alignment);
		CAGE_ASSERT((uintPtr(res) % alignment) == 0);
		return res;
	}

	void MemoryArena::deallocate(void *ptr)
	{
		stub->dealloc(inst, ptr);
	}

	void MemoryArena::flush()
	{
		stub->fls(inst);
	}

	namespace
	{
		void *malloca(uintPtr size, uintPtr alignment)
		{
			CAGE_ASSERT(size > 0);
			CAGE_ASSERT(detail::isPowerOf2(alignment));
			const uintPtr s = detail::roundUpTo(size, alignment);
#ifdef _MSC_VER
			void *p = _aligned_malloc(s, alignment);
#else
			void *p = std::aligned_alloc(alignment, s);
#endif // _MSC_VER
			CAGE_ASSERT(uintPtr(p) % alignment == 0);
			return p;
		}

		void freea(void *ptr)
		{
#ifdef _MSC_VER
			_aligned_free(ptr);
#else
			::free(ptr);
#endif // _MSC_VER
		}

		class SystemMemoryArenaImpl : private Immovable
		{
		public:
			void *allocate(uintPtr size, uintPtr alignment)
			{
				void *tmp = malloca(size, alignment);
				if (!tmp)
					CAGE_THROW_ERROR(OutOfMemory, "system memory arena out of memory", size);
				allocations++;
				return tmp;
			}

			void deallocate(void *ptr)
			{
				if (!ptr)
					return;
				freea(ptr);
				if (allocations-- == 0)
					CAGE_THROW_CRITICAL(Exception, "memory corruption - double deallocation detected");
			}

			void flush()
			{
				CAGE_THROW_CRITICAL(Exception, "invalid operation - deallocate must be used");
			}

			MemoryArena arena = MemoryArena(this);

			std::atomic<uint32> allocations = 0;
		};
	}

	MemoryArena &systemMemory()
	{
		static SystemMemoryArenaImpl *arena = new SystemMemoryArenaImpl(); // intentionally left to leak
		return arena->arena;
	}
}
