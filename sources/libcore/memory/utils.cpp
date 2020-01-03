#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <zlib.h>

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

	MemoryArena::MemoryArena() noexcept : stub(nullptr), inst(nullptr)
	{}

	void *MemoryArena::allocate(uintPtr size, uintPtr alignment)
	{
		void *res = stub->alloc(inst, size, alignment);
		CAGE_ASSERT((uintPtr(res) % alignment) == 0, size, alignment, res, uintPtr(res) % alignment);
		return res;
	}

	void MemoryArena::deallocate(void *ptr) noexcept
	{
		stub->dealloc(inst, ptr);
	}

	void MemoryArena::flush() noexcept
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
				std::atomic<sint32> cnt = 1;
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

		void incHolderShareable(void *ptr, const Delegate<void(void *)> &deleter)
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
		uintPtr compressionBound(uintPtr size)
		{
			return ::compressBound(numeric_cast<uLong>(size));
		}

		uintPtr compress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize)
		{
			uLongf s = numeric_cast<uLongf>(outputSize);
			int level = numeric_cast<int>(Z_BEST_COMPRESSION);
			int res = ::compress2((Bytef*)outputBuffer, &s, (const Bytef*)inputBuffer, numeric_cast<uLong>(inputSize), level);
			switch (res)
			{
			case Z_OK:
				return s;
			case Z_MEM_ERROR: // some Allocation failed
				CAGE_THROW_ERROR(Exception, "compression failed with allocation error");
			case Z_BUF_ERROR: // output buffer was too small
				CAGE_THROW_ERROR(OutOfMemory, "output buffer for compression is too small", 0);
			case Z_STREAM_ERROR:
				CAGE_THROW_CRITICAL(Exception, "invalid compression level");
			default:
				CAGE_THROW_CRITICAL(Exception, "compression failed with unknown status");
			}
		}

		uintPtr decompress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize)
		{
			uLongf s = numeric_cast<uLongf>(outputSize);
			int res = ::uncompress((Bytef*)outputBuffer, &s, (const Bytef*)inputBuffer, numeric_cast<uLong>(inputSize));
			switch (res)
			{
			case Z_OK:
				return s;
			case Z_MEM_ERROR: // some Allocation failed
				CAGE_THROW_ERROR(Exception, "decompression failed with allocation error");
			case Z_BUF_ERROR: // output buffer was too small
				CAGE_THROW_ERROR(OutOfMemory, "output buffer for decompression is too small", 0);
			case Z_DATA_ERROR:
				CAGE_THROW_ERROR(Exception, "input buffer for decompression is corrupted");
			default:
				CAGE_THROW_CRITICAL(Exception, "decompression failed with unknown status");
			}
		}

		namespace
		{
			void *malloca(uintPtr size, uintPtr alignment)
			{
				CAGE_ASSERT(isPowerOf2(alignment), "impossible alignment", alignment);
				void *p = ::malloc(size + alignment - 1 + sizeof(void*));
				if (!p)
					return nullptr;
				void *ptr = (void*)((uintPtr(p) + sizeof(void*) + alignment - 1) & ~(alignment - 1));
				*((void **)ptr - 1) = p;
				CAGE_ASSERT(uintPtr(ptr) % alignment == 0, ptr, alignment);
				return ptr;
			}

			void freea(void *ptr)
			{
				if (!ptr)
					return;
				void *p = *((void **)ptr - 1);
				::free(p);
			}

			class Memory1Impl
			{
			public:
				Memory1Impl() : allocations(0)
				{}

				~Memory1Impl()
				{
					try
					{
						if (allocations != 0)
							CAGE_THROW_CRITICAL(Exception, "memory corruption - memory leak detected");
					}
					catch (...)
					{}
				}

				void *allocate(uintPtr size, uintPtr alignment)
				{
					void *tmp = malloca(size, alignment);
					if (!tmp)
						CAGE_THROW_ERROR(OutOfMemory, "out of memory", size);
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

				std::atomic<uint32> allocations;
			};
		}

		MemoryArena &systemArena()
		{
			static Memory1Impl *impl = new Memory1Impl(); // intentionally left to leak
			static MemoryArena *arena = new MemoryArena(impl);
			return *arena;
		}
	}

	namespace
	{
		class VirtualMemoryImpl : public VirtualMemory
		{
		public:
			VirtualMemoryImpl() : origin(nullptr), pgs(0), total(0), pageSize(detail::memoryPageSize())
			{}

			~VirtualMemoryImpl()
			{
				free();
			}

			void *reserve(const uintPtr pages)
			{
				CAGE_ASSERT(pages > 0, "virtualMemory::reserve: zero pages not allowed", pages);
				CAGE_ASSERT(!origin, "virtualMemory::reserve: already reserved");
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
				CAGE_ASSERT(pages > 0, "invalid value - zero pages", pages);
				CAGE_ASSERT(origin, "invalid operation - first reserve");

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
				CAGE_ASSERT(pages > 0, "invalid value - zero pages", pages);
				CAGE_ASSERT(origin, "invalid operation - first reserve");
				CAGE_ASSERT(pages < pgs, "invalid value - too few left", pages, pgs);

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualFree((char*)origin + pageSize * (pgs - pages), pageSize * pages, MEM_DECOMMIT))
					CAGE_THROW_ERROR(Exception, "VirtualFree");
#else
				if (!mprotect((char*)origin + pageSize * (pgs - pages), pageSize * pages, PROT_NONE))
					CAGE_THROW_ERROR(SystemError, "mprotect", errno);
#endif

				pgs -= pages;
			}

			void *origin;
			uintPtr pgs, total;
			const uintPtr pageSize;
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
		namespace
		{
			uintPtr pageSizeInitializer()
			{
#ifdef CAGE_SYSTEM_WINDOWS

				SYSTEM_INFO info;
				GetSystemInfo(&info);
				return info.dwPageSize;

#else
				return sysconf(_SC_PAGE_SIZE);
#endif
			}
		}

		uintPtr memoryPageSize()
		{
			static const uintPtr pagesize = pageSizeInitializer();
			return pagesize;
		}
	}
}
