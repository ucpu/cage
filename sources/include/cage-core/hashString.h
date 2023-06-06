#ifndef guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
#define guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_

#include "hashBuffer.h"

namespace cage
{
	// HashString returns values in upper half of uint32 range
	struct CAGE_CORE_API HashString
	{
		CAGE_FORCE_INLINE constexpr explicit HashString(const char *str) noexcept : value(hashRawString(str) | ((uint32)1 << 31)) {}

		CAGE_FORCE_INLINE constexpr explicit HashString(const StringPointer &str) noexcept : value(hashRawString(str) | ((uint32)1 << 31)) {}

		template<uint32 N>
		CAGE_FORCE_INLINE constexpr explicit HashString(const detail::StringBase<N> &str) noexcept : value(hashBuffer(str) | ((uint32)1 << 31))
		{}

		CAGE_FORCE_INLINE constexpr operator uint32() const noexcept { return value; }

	private:
		uint32 value = 0;
	};
}

#endif // guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
