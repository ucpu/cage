#include <cage-core/memoryUtils.h>

#ifdef CAGE_SYSTEM_WINDOWS
	#include "../incWin.h"
	#include <malloc.h>
#else
	#include <sys/mman.h>
	#include <unistd.h>
#endif

#if __has_include(<valgrind/helgrind.h>)
    // see https://valgrind.org/docs/manual/hg-manual.html
	#include <valgrind/helgrind.h>
#else
	#define ANNOTATE_HAPPENS_AFTER(XXX)
	#define ANNOTATE_HAPPENS_BEFORE_FORGET_ALL(XXX)
	#define ANNOTATE_HAPPENS_BEFORE(XXX)
#endif

#include <cerrno>

namespace cage
{
	static_assert(sizeof(Holder<uint32>) == 2 * sizeof(uintPtr));
	static_assert(sizeof(Holder<String>) == 2 * sizeof(uintPtr));

	OutOfMemory::OutOfMemory(const std::source_location &location, SeverityEnum severity, StringPointer message, uintPtr memory) noexcept : Exception(location, severity, message), memory(memory){};

	void OutOfMemory::log() const
	{
		::cage::privat::makeLog(location, SeverityEnum::Note, "exception", Stringizer() + "memory requested: " + memory, false, false);
		::cage::privat::makeLog(location, severity, "exception", +message, false, false);
	};

	namespace privat
	{
		// platform specific implementation to avoid including <atomic> in the core.h

		void HolderControlBase::inc()
		{
			const uint32 val =
#ifdef CAGE_SYSTEM_WINDOWS
			    InterlockedIncrement(&counter);
#else
			    __atomic_add_fetch(&counter, (uint32)1, __ATOMIC_SEQ_CST);
#endif // CAGE_SYSTEM_WINDOWS
			CAGE_ASSERT(val != m);
		}

		void HolderControlBase::dec()
		{
			const uint32 val =
#ifdef CAGE_SYSTEM_WINDOWS
			    InterlockedDecrement(&counter);
#else
			    __atomic_sub_fetch(&counter, (uint32)1, __ATOMIC_SEQ_CST);
#endif // CAGE_SYSTEM_WINDOWS
			CAGE_ASSERT(val != m);

			if (val == 0)
			{
				ANNOTATE_HAPPENS_AFTER(&counter);
				ANNOTATE_HAPPENS_BEFORE_FORGET_ALL(&counter);
				deleter(deletee);
			}
			else
			{
				ANNOTATE_HAPPENS_BEFORE(&counter);
			}
		}
	}

	namespace
	{
		class VirtualMemoryImpl : public VirtualMemory
		{
		public:
			VirtualMemoryImpl() {}

			~VirtualMemoryImpl() { free(); }

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
				if (!VirtualAlloc((char *)origin + pgs * pageSize, pages * pageSize, MEM_COMMIT, PAGE_READWRITE))
					CAGE_THROW_ERROR(Exception, "VirtualAlloc");
#else
				if (mprotect((char *)origin + pgs * pageSize, pages * pageSize, PROT_READ | PROT_WRITE) != 0)
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
				if (!VirtualFree((char *)origin + pageSize * (pgs - pages), pageSize * pages, MEM_DECOMMIT))
					CAGE_THROW_ERROR(Exception, "VirtualFree");
#else
				if (!mprotect((char *)origin + pageSize * (pgs - pages), pageSize * pages, PROT_NONE))
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
		VirtualMemoryImpl *impl = (VirtualMemoryImpl *)this;
		return impl->reserve(pages);
	}

	void VirtualMemory::free()
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl *)this;
		impl->free();
	}

	void VirtualMemory::increase(uintPtr pages)
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl *)this;
		impl->increase(pages);
	}

	void VirtualMemory::decrease(uintPtr pages)
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl *)this;
		impl->decrease(pages);
	}

	uintPtr VirtualMemory::pages() const
	{
		VirtualMemoryImpl *impl = (VirtualMemoryImpl *)this;
		return impl->pgs;
	}

	Holder<VirtualMemory> newVirtualMemory()
	{
		return systemMemory().createImpl<VirtualMemory, VirtualMemoryImpl>();
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
