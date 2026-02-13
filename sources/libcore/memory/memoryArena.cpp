#include <mimalloc.h>

#include <cage-core/memoryArena.h>
#include <cage-core/memoryUtils.h> // isPowerOf2

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

	Holder<PointerRange<char>> MemoryArena::createBuffer(uintPtr size, uintPtr alignment)
	{
		struct CharBuff : public privat::HolderControlBase
		{
			PointerRange<char> data;
		};
		char *base = (char *)allocate(sizeof(CharBuff) + size + alignment - 1, alignof(CharBuff));
		CAGE_ASSERT(base);
		CharBuff *cb = new (base, privat::OperatorNewTrait()) CharBuff();
		CAGE_ASSERT(cb);
		CAGE_ASSERT((void *)base == (void *)cb);
		cb->deletee = cb;
		cb->deleter.template bind<MemoryArena, &MemoryArena::destroy<CharBuff>>(this);
		char *start = base + sizeof(CharBuff);
		start += detail::addToAlign((uintPtr)start, alignment);
		CAGE_ASSERT(((uintPtr)start % alignment) == 0);
		cb->data = PointerRange<char>(start, start + size);
		return Holder<PointerRange<char>>(&cb->data, cb);
	}

	namespace
	{
		class SystemMemoryArenaImpl : private Immovable
		{
		public:
			void *allocate(uintPtr size, uintPtr alignment)
			{
				void *tmp = mi_malloc_aligned(size, alignment);
				if (!tmp)
					CAGE_THROW_ERROR(OutOfMemory, "system memory arena out of memory", size);
				return tmp;
			}

			void deallocate(void *ptr)
			{
				if (!ptr)
					return;
				mi_free(ptr);
			}

			void flush() { CAGE_THROW_CRITICAL(Exception, "invalid operation (flush on system memory arena), deallocate must be used instead"); }

			MemoryArena arena = MemoryArena(this);
		};
	}

	MemoryArena &systemMemory()
	{
		static SystemMemoryArenaImpl *arena = new SystemMemoryArenaImpl(); // intentionally left to leak
		return arena->arena;
	}
}
