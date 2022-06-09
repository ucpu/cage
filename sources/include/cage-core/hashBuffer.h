#ifndef guard_hashBuffer_h_io45uuzjhd8s5xfgh
#define guard_hashBuffer_h_io45uuzjhd8s5xfgh

#include "core.h"

namespace cage
{
	namespace detail
	{
		constexpr uint32 HashOffset = 2166136261u;
		constexpr uint32 HashPrime = 16777619u;
	}

	CAGE_FORCE_INLINE constexpr uint32 hashBuffer(PointerRange<const char> buffer) noexcept
	{
		const char *b = buffer.begin();
		const char *e = buffer.end();
		uint32 hash = detail::HashOffset;
		while (b != e)
		{
			hash ^= *b++;
			hash *= detail::HashPrime;
		}
		return hash;
	}

	CAGE_FORCE_INLINE constexpr uint32 hashRawString(const char *str) noexcept
	{
		const char *e = str;
		while (*e)
			e++;
		return hashBuffer({ str, e });
	}
}

#endif // guard_hashBuffer_h_io45uuzjhd8s5xfgh
