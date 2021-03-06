#ifndef guard_memoryUtils_h_dsh45s56ed4hs94rt4ezhk47op
#define guard_memoryUtils_h_dsh45s56ed4hs94rt4ezhk47op

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API OutOfMemory : public Exception
	{
		explicit OutOfMemory(StringLiteral function, StringLiteral file, uint32 line, SeverityEnum severity, StringLiteral message, uintPtr memory) noexcept;
		void log() override;
		uintPtr memory = 0;
	};

	namespace detail
	{
		inline bool isPowerOf2(uintPtr x)
		{
			return x && !(x & (x - 1));
		}

		inline uintPtr addToAlign(uintPtr ptr, uintPtr alignment)
		{
			return (alignment - (ptr % alignment)) % alignment;
		}

		inline uintPtr subtractToAlign(uintPtr ptr, uintPtr alignment)
		{
			return (alignment - addToAlign(ptr, alignment)) % alignment;
		}

		inline uintPtr roundDownTo(uintPtr ptr, uintPtr alignment)
		{
			return (ptr / alignment) * alignment;
		}

		inline uintPtr roundUpTo(uintPtr ptr, uintPtr alignment)
		{
			return roundDownTo(ptr + alignment - 1, alignment);
		}

		CAGE_CORE_API uintPtr memoryPageSize();
	}

	class CAGE_CORE_API VirtualMemory : private Immovable
	{
	public:
		void *reserve(uintPtr pages); // reserve address space
		void free(); // give up whole address space
		void increase(uintPtr pages); // allocate pages
		void decrease(uintPtr pages); // give up pages
		uintPtr pages() const; // currently allocated pages
	};

	CAGE_CORE_API Holder<VirtualMemory> newVirtualMemory();
}

#endif // guard_memoryUtils_h_dsh45s56ed4hs94rt4ezhk47op
