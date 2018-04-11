#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <cerrno>

namespace cage
{
	namespace
	{
		class virtualMemoryImpl : public virtualMemoryClass
		{
		public:
			virtualMemoryImpl() : origin(nullptr), pgs(0), total(0)
			{}

			~virtualMemoryImpl()
			{
				free();
			}

			void *reserve(const uintPtr pages)
			{
				CAGE_ASSERT_RUNTIME(pages > 0, "virtualMemory::reserve: zero pages not allowed", pages);
				CAGE_ASSERT_RUNTIME(!origin, "virtualMemory::reserve: already reserved");
				total = pages;
				pgs = 0;

#ifdef CAGE_SYSTEM_WINDOWS

				origin = VirtualAlloc(nullptr, pages * detail::memoryPageSize(), MEM_RESERVE, PAGE_NOACCESS);
				if (!origin)
					CAGE_THROW_ERROR(exception, "VirtualAlloc");

#else

				origin = mmap(nullptr, pages * detail::memoryPageSize(), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (origin == MAP_FAILED)
					CAGE_THROW_ERROR(codeException, "mmap", errno);

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
				if (munmap(origin, total * detail::memoryPageSize()) != 0)
					CAGE_THROW_ERROR(codeException, "munmap", errno);
#endif

				origin = nullptr;
				pgs = total = 0;
			}

			void increase(uintPtr pages)
			{
				CAGE_ASSERT_RUNTIME(pages > 0, "invalid value - zero pages", pages);
				CAGE_ASSERT_RUNTIME(origin, "invalid operation - first reserve");

				if (pages + pgs > total)
					CAGE_THROW_CRITICAL(exception, "virtual memory depleted");

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualAlloc((char*)origin + pgs * detail::memoryPageSize(), pages * detail::memoryPageSize(), MEM_COMMIT, PAGE_READWRITE))
					CAGE_THROW_ERROR(exception, "VirtualAlloc");
#else
				if (mprotect((char*)origin + pgs * detail::memoryPageSize(), pages * detail::memoryPageSize(), PROT_READ | PROT_WRITE) != 0)
					CAGE_THROW_ERROR(codeException, "mprotect", errno);
#endif

				pgs += pages;
			}

			void decrease(uintPtr pages)
			{
				CAGE_ASSERT_RUNTIME(pages > 0, "invalid value - zero pages", pages);
				CAGE_ASSERT_RUNTIME(origin, "invalid operation - first reserve");
				CAGE_ASSERT_RUNTIME(pages < pgs, "invalid value - too few left", pages, pgs);

#ifdef CAGE_SYSTEM_WINDOWS
				if (!VirtualFree((char*)origin + detail::memoryPageSize() * (pgs - pages), detail::memoryPageSize() * pages, MEM_DECOMMIT))
					CAGE_THROW_ERROR(exception, "VirtualFree");
#else
				if (!mprotect((char*)origin + detail::memoryPageSize() * (pgs - pages), detail::memoryPageSize() * pages, PROT_NONE))
					CAGE_THROW_ERROR(codeException, "mprotect", errno);
#endif

				pgs -= pages;
			}

			void *origin;
			uintPtr pgs, total;
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
		return detail::systemArena().createImpl <virtualMemoryClass, virtualMemoryImpl>();
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
