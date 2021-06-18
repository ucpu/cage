#ifndef guard_memoryArena_h_yysrtdf56hhuio784hg12edr
#define guard_memoryArena_h_yysrtdf56hhuio784hg12edr

#include "core.h"

namespace cage
{
	namespace privat
	{
		struct CAGE_CORE_API MemoryArenaStub
		{
			void *(*alloc)(void *, uintPtr, uintPtr);
			void (*dealloc)(void *, void *);
			void (*fls)(void *);

			template<class A>
			CAGE_FORCE_INLINE static void *allocate(void *inst, uintPtr size, uintPtr alignment)
			{
				return ((A *)inst)->allocate(size, alignment);
			}

			template<class A>
			CAGE_FORCE_INLINE static void deallocate(void *inst, void *ptr)
			{
				((A *)inst)->deallocate(ptr);
			}

			template<class A>
			CAGE_FORCE_INLINE static void flush(void *inst)
			{
				((A *)inst)->flush();
			}

			template<class A>
			static MemoryArenaStub init() noexcept
			{
				MemoryArenaStub s;
				s.alloc = &allocate<A>;
				s.dealloc = &deallocate<A>;
				s.fls = &flush<A>;
				return s;
			}
		};
	}

	template<class A>
	inline MemoryArena::MemoryArena(A *a) noexcept
	{
		static privat::MemoryArenaStub stb = privat::MemoryArenaStub::init<A>();
		this->stub = &stb;
		inst = a;
	}
}

#endif // guard_memoryArena_h_yysrtdf56hhuio784hg12edr
