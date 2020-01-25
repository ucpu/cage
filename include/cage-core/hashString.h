#ifndef guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
#define guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_

#define GCHL_HASH_OFFSET 2166136261u
#define GCHL_HASH_PRIME 16777619u

namespace cage
{
	namespace detail
	{
		// these functions return values in the full range of uint32
		CAGE_API uint32 hashBuffer(const char *buffer, uintPtr size);
		CAGE_API uint32 hashBuffer(const MemoryBuffer &buffer);
		CAGE_API uint32 HashString(const char *str);

		// HashCompile returns values in the full range of uint32
		struct CAGE_API HashCompile
		{
		private:
			uint32 value;

			template<uint32 N, uint32 I>
			struct Hash
			{
				constexpr uint32 operator ()(const char(&str)[N]) noexcept
				{
					return (Hash<N, I - 1>()(str) ^ str[I - 2]) * GCHL_HASH_PRIME;
				}
			};

			template<uint32 N>
			struct Hash<N, 1>
			{
				constexpr uint32 operator ()(const char(&str)[N]) noexcept
				{
					return GCHL_HASH_OFFSET;
				}
			};

		public:
			template<uint32 N>
			explicit HashCompile(const char(&str)[N]) noexcept : value(Hash<N, N>()(str))
			{};

			operator uint32() const noexcept
			{
				return value;
			}
		};
	}

	// HashString returns values in (upper) half of uint32 range
	struct CAGE_API HashString
	{
	private:
		struct ConstCharWrapper
		{
			ConstCharWrapper(const char *str) : str(str) {}
			const char *str;
		};

		uint32 value;

	public:
		explicit HashString(const ConstCharWrapper &wrp) : value(detail::HashString(wrp.str) | ((uint32)1 << 31))
		{}

		template<uint32 N>
		explicit HashString(const detail::StringBase<N> &str) : HashString(str.c_str())
		{}

		template<uint32 N>
		explicit HashString(const char(&str)[N]) noexcept : value(detail::HashCompile(str) | ((uint32)1 << 31))
		{}

		operator uint32() const noexcept
		{
			return value;
		}
	};
}

#endif // guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
