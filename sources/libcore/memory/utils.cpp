#include <cstdlib>
#include <atomic>
#include <cerrno>
#include <zlib.h>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

namespace cage
{
	outOfMemory::outOfMemory(const char *file, uint32 line, const char *function, severityEnum severity, const char *message, uintPtr memory) noexcept : exception(file, line, function, severity, message), memory(memory)
	{};

	void outOfMemory::log()
	{
		::cage::privat::makeLog(file, line, function, severityEnum::Note, "exception", stringizer() + "memory requested: " + memory, false, false);
		::cage::privat::makeLog(file, line, function, severity, "exception", message, false, false);
	};

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
			case Z_MEM_ERROR: // some allocation failed
				CAGE_THROW_ERROR(exception, "compression failed with allocation error");
			case Z_BUF_ERROR: // output buffer was too small
				CAGE_THROW_ERROR(outOfMemory, "output buffer for compression is too small", 0);
			case Z_STREAM_ERROR:
				CAGE_THROW_CRITICAL(exception, "invalid compression level");
			default:
				CAGE_THROW_CRITICAL(exception, "compression failed with unknown status");
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
			case Z_MEM_ERROR: // some allocation failed
				CAGE_THROW_ERROR(exception, "decompression failed with allocation error");
			case Z_BUF_ERROR: // output buffer was too small
				CAGE_THROW_ERROR(outOfMemory, "output buffer for decompression is too small", 0);
			case Z_DATA_ERROR:
				CAGE_THROW_ERROR(exception, "input buffer for decompression is corrupted");
			default:
				CAGE_THROW_CRITICAL(exception, "decompression failed with unknown status");
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

			class memory1Impl
			{
			public:
				memory1Impl() : allocations(0)
				{}

				~memory1Impl()
				{
					try
					{
						if (allocations != 0)
							CAGE_THROW_CRITICAL(exception, "memory corruption - memory leak detected");
					}
					catch (...)
					{}
				}

				void *allocate(uintPtr size, uintPtr alignment)
				{
					void *tmp = malloca(size, alignment);
					if (!tmp)
						CAGE_THROW_ERROR(outOfMemory, "out of memory", size);
					allocations++;
					return tmp;
				}

				void deallocate(void *ptr)
				{
					if (!ptr)
						return;
					freea(ptr);
					if (allocations-- == 0)
						CAGE_THROW_CRITICAL(exception, "memory corruption - double deallocation detected");
				}

				void flush()
				{
					CAGE_THROW_CRITICAL(exception, "invalid operation - deallocate must be used");
				}

				std::atomic<uint32> allocations;
			};
		}

		memoryArena &systemArena()
		{
			static memory1Impl *impl = new memory1Impl(); // intentionally left to leak
			static memoryArena *arena = new memoryArena(impl);
			return *arena;
		}
	}

	namespace
	{
		class virtualMemoryImpl : public virtualMemoryClass
		{
		public:
			virtualMemoryImpl() : origin(nullptr), pgs(0), total(0), pageSize(detail::memoryPageSize())
			{}

			~virtualMemoryImpl()
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
					CAGE_THROW_ERROR(exception, "VirtualAlloc");

#else

				origin = mmap(nullptr, pages * pageSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (origin == MAP_FAILED)
					CAGE_THROW_ERROR(systemError, "mmap", errno);

#endif

				return origin;
			}

			void free()
			{
				if (!origin)
					return;

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualFree(origin, 0, MEM_RELEASE))
					CAGE_THROW_ERROR(exception, "VirtualFree");
#else
				if (munmap(origin, total * pageSize) != 0)
					CAGE_THROW_ERROR(systemError, "munmap", errno);
#endif

				origin = nullptr;
				pgs = total = 0;
			}

			void increase(uintPtr pages)
			{
				CAGE_ASSERT(pages > 0, "invalid value - zero pages", pages);
				CAGE_ASSERT(origin, "invalid operation - first reserve");

				if (pages + pgs > total)
					CAGE_THROW_CRITICAL(exception, "virtual memory depleted");

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualAlloc((char*)origin + pgs * pageSize, pages * pageSize, MEM_COMMIT, PAGE_READWRITE))
					CAGE_THROW_ERROR(exception, "VirtualAlloc");
#else
				if (mprotect((char*)origin + pgs * pageSize, pages * pageSize, PROT_READ | PROT_WRITE) != 0)
					CAGE_THROW_ERROR(systemError, "mprotect", errno);
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
					CAGE_THROW_ERROR(exception, "VirtualFree");
#else
				if (!mprotect((char*)origin + pageSize * (pgs - pages), pageSize * pages, PROT_NONE))
					CAGE_THROW_ERROR(systemError, "mprotect", errno);
#endif

				pgs -= pages;
			}

			void *origin;
			uintPtr pgs, total;
			const uintPtr pageSize;
		};
	}

	void *virtualMemoryClass::reserve(uintPtr pages)
	{
		virtualMemoryImpl *impl = (virtualMemoryImpl*)this;
		return impl->reserve(pages);
	}

	void virtualMemoryClass::free()
	{
		virtualMemoryImpl *impl = (virtualMemoryImpl*)this;
		impl->free();
	}

	void virtualMemoryClass::increase(uintPtr pages)
	{
		virtualMemoryImpl *impl = (virtualMemoryImpl*)this;
		impl->increase(pages);
	}

	void virtualMemoryClass::decrease(uintPtr pages)
	{
		virtualMemoryImpl *impl = (virtualMemoryImpl*)this;
		impl->decrease(pages);
	}

	uintPtr virtualMemoryClass::pages() const
	{
		virtualMemoryImpl *impl = (virtualMemoryImpl*)this;
		return impl->pgs;
	}

	holder<virtualMemoryClass> newVirtualMemory()
	{
		return detail::systemArena().createImpl<virtualMemoryClass, virtualMemoryImpl>();
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
