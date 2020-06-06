#ifndef guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
#define guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_

#include "core.h"

namespace cage
{
	namespace detail
	{
		// these functions return values in the full range of uint32
		CAGE_CORE_API uint32 hashBuffer(PointerRange<const char> buffer) noexcept;
		CAGE_CORE_API uint32 hashString(const char *str) noexcept;

		// HashCompile returns values in the full range of uint32
		struct CAGE_CORE_API HashCompile
		{
		private:
			uint32 value;

			template<uint32 N, uint32 I>
			struct Hash
			{
				constexpr uint32 operator ()(const char(&str)[N]) const noexcept
				{
					return (Hash<N, I - 1>()(str) ^ str[I - 2]) * Prime;
				}
			};

			template<uint32 N>
			struct Hash<N, 1>
			{
				constexpr uint32 operator ()(const char(&str)[N]) const noexcept
				{
					return Offset;
				}
			};

		public:
			template<uint32 N>
			constexpr explicit HashCompile(const char(&str)[N]) noexcept : value(Hash<N, N>()(str))
			{};

			constexpr operator uint32() const noexcept
			{
				return value;
			}

			static constexpr uint32 Offset = 2166136261u;
			static constexpr uint32 Prime = 16777619u;
		};
	}

	// HashString returns values in upper half of uint32 range
	struct CAGE_CORE_API HashString
	{
	private:
		struct ConstCharWrapper
		{
			ConstCharWrapper(const char *str) noexcept : str(str) {}
			const char *str;
		};

		uint32 value;

	public:
		explicit HashString(const ConstCharWrapper &wrp) noexcept : value(detail::hashString(wrp.str) | ((uint32)1 << 31))
		{}

		template<uint32 N>
		explicit HashString(const detail::StringBase<N> &str) noexcept : HashString(str.c_str())
		{}

		template<uint32 N>
		constexpr explicit HashString(const char(&str)[N]) noexcept : value(detail::HashCompile(str) | ((uint32)1 << 31))
		{}

		constexpr operator uint32() const noexcept
		{
			return value;
		}
	};
}

#endif // guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
