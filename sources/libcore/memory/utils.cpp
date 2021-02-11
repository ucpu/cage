#include <cage-core/memoryUtils.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#include <malloc.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <cstdlib>
#include <atomic>
#include <cerrno>

namespace cage
{
	OutOfMemory::OutOfMemory(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uintPtr memory) noexcept : Exception(file, line, function, severity, message), memory(memory)
	{};

	void OutOfMemory::log()
	{
		::cage::privat::makeLog(file, line, function, SeverityEnum::Note, "exception", stringizer() + "memory requested: " + memory, false, false);
		::cage::privat::makeLog(file, line, function, severity, "exception", message, false, false);
	};

	MemoryArena::MemoryArena() : stub(nullptr), inst(nullptr)
	{}

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

	bool MemoryArena::operator == (const MemoryArena &other) const noexcept
	{
		return inst == other.inst;
	}

	bool MemoryArena::operator != (const MemoryArena &other) const noexcept
	{
		return !(*this == other);
	}

	namespace privat
	{
		namespace
		{
			struct SharedCounter
			{
				std::atomic<sint32> cnt{1};
				Holder<void> payload;

				void dec()
				{
					if (--cnt <= 0)
					{
						detail::systemArena().destroy<SharedCounter>(this); // suicide
					}
				}
			};

			void decHolderShareable(void *ptr)
			{
				((SharedCounter*)ptr)->dec();
			}
		}

		bool isHolderShareable(const Delegate<void(void *)> &deleter)
		{
			return deleter == Delegate<void(void *)>().bind<&decHolderShareable>();
		}

		void incrementHolderShareable(void *ptr, const Delegate<void(void *)> &deleter)
		{
			CAGE_ASSERT(isHolderShareable(deleter));
			((SharedCounter*)ptr)->cnt++;
		}

		void makeHolderShareable(void *&ptr, Delegate<void(void *)> &deleter)
		{
			if (isHolderShareable(deleter))
				return;
			SharedCounter *shr = detail::systemArena().createObject<SharedCounter>();
			shr->payload = Holder<void>(nullptr, ptr, deleter);
			ptr = shr;
			deleter = Delegate<void(void *)>().bind<&decHolderShareable>();
		}
	}

	namespace detail
	{
		namespace
		{
			void *malloca(uintPtr size, uintPtr alignment)
			{
				CAGE_ASSERT(size > 0);
				CAGE_ASSERT(isPowerOf2(alignment));
				uintPtr s = roundUpTo(size, alignment);
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
				SystemMemoryArenaImpl() : arena(this)
				{}

				void *allocate(uintPtr size, uintPtr alignment)
				{
					void *tmp = malloca(size, alignment);
					if (!tmp)
						CAGE_THROW_ERROR(OutOfMemory, "system arena out of memory", size);
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

				std::atomic<uint32> allocations = 0;

				MemoryArena arena;
			};
		}

		MemoryArena &systemArena()
		{
			static SystemMemoryArenaImpl *arena = new SystemMemoryArenaImpl(); // intentionally left to leak
			return arena->arena;
		}
	}

	namespace
	{
		class VirtualMemoryImpl : public VirtualMemory
		{
		public:
			VirtualMemoryImpl()
			{}

			~VirtualMemoryImpl()
			{
				free();
			}

			void *reserve(const uintPtr pages)
			{
				CAGE_ASSERT(pages > 0);
				CAGE_ASSERT(!origin);
				total = pages;
				pgs = 0;

#ifdef CAGE_SYSTEM_WINDOWS

				origin = VirtualAlloc(nullptr, pages * pageSize, MEM_RESERVE, PAGE_NOACCESS);
				if (!origin)
					CAGE_THROW_ERROR(Exception, "VirtualAlloc");

#else

				origin = mmap(nullptr, pages * pageSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (origin == MAP_FAILED)
					CAGE_THROW_ERROR(SystemError, "mmap", errno);

#endif

				return origin;
			}

			void free()
			{
				if (!origin)
					return;

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualFree(origin, 0, MEM_RELEASE))
					CAGE_THROW_ERROR(Exception, "VirtualFree");
#else
				if (munmap(origin, total * pageSize) != 0)
					CAGE_THROW_ERROR(SystemError, "munmap", errno);
#endif

				origin = nullptr;
				pgs = total = 0;
			}

			void increase(uintPtr pages)
			{
				CAGE_ASSERT(pages > 0);
				CAGE_ASSERT(origin);

				if (pages + pgs > total)
					CAGE_THROW_CRITICAL(Exception, "virtual memory depleted");

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualAlloc((char*)origin + pgs * pageSize, pages * pageSize, MEM_COMMIT, PAGE_READWRITE))
					CAGE_THROW_ERROR(Exception, "VirtualAlloc");
#else
				if (mprotect((char*)origin + pgs * pageSize, pages * pageSize, PROT_READ | PROT_WRITE) != 0)
					CAGE_THROW_ERROR(SystemError, "mprotect", errno);
#endif

				pgs += pages;
			}

			void decrease(uintPtr pages)
			{
				CAGE_ASSERT(pages > 0);
				CAGE_ASSERT(origin);
				CAGE_ASSERT(pages < pgs);

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualFree((char*)origin + pageSize * (pgs - pages), pageSize * pages, MEM_DECOMMIT))
					CAGE_THROW_ERROR(Exception, "VirtualFree");
#else
				if (!mprotect((char*)origin + pageSize * (pgs - pages), pageSize * pages, PROT_NONE))
					CAGE_THROW_ERROR(SystemError, "mprotect", errno);
#endif

				pgs -= pages;
			}

			void *origin = nullptr;
			uintPtr pgs = 0, total = 0;
			const uintPtr pageSize = detail::memoryPageSize();
		};
	}

	void *VirtualMemory::reserve(uintPtr pages)
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl*)this;
		return impl->reserve(pages);
	}

	void VirtualMemory::free()
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl*)this;
		impl->free();
	}

	void VirtualMemory::increase(uintPtr pages)
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl*)this;
		impl->increase(pages);
	}

	void VirtualMemory::decrease(uintPtr pages)
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl*)this;
		impl->decrease(pages);
	}

	uintPtr VirtualMemory::pages() const
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl*)this;
		return impl->pgs;
	}

	Holder<VirtualMemory> newVirtualMemory()
	{
		return detail::systemArena().createImpl<VirtualMemory, VirtualMemoryImpl>();
	}

	namespace detail
	{
		uintPtr memoryPageSize()
		{
			static const uintPtr pagesize = []()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				SYSTEM_INFO info;
				GetSystemInfo(&info);
				return info.dwPageSize;
#else
				return sysconf(_SC_PAGE_SIZE);
#endif
			}();
			return pagesize;
		}
	}
}
