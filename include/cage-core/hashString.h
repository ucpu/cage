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
		CAGE_API uint32 hashBuffer(const memoryBuffer &buffer);
		CAGE_API uint32 hashString(const char *str);

		// hashCompile returns values in the full range of uint32
		struct CAGE_API hashCompile
		{
		private:
			uint32 value;

			template<uint32 N, uint32 I>
			struct hash
			{
				constexpr uint32 operator ()(const char(&str)[N])
				{
					return (hash<N, I - 1>()(str) ^ str[I - 2]) * GCHL_HASH_PRIME;
				}
			};

			template<uint32 N>
			struct hash<N, 1>
			{
				constexpr uint32 operator ()(const char(&str)[N])
				{
					return GCHL_HASH_OFFSET;
				}
			};

		public:
			template<uint32 N>
			explicit hashCompile(const char(&str)[N]) : value(hash<N, N>()(str))
			{};

			operator uint32() const
			{
				return value;
			}
		};
	}

	// hashString returns values in (upper) half of uint32 range
	struct CAGE_API hashString
	{
	private:
		struct constCharWrapper
		{
			constCharWrapper(const char *str) : str(str) {};
			const char *str;
		};

		uint32 value;

	public:
		explicit hashString(const constCharWrapper &wrp) : value(detail::hashString(wrp.str) | ((uint32)1 << 31))
		{};

		template<uint32 N>
		explicit hashString(const detail::stringBase<N> &str) : hashString(str.c_str())
		{}

		template<uint32 N>
		explicit hashString(const char(&str)[N]) : value(detail::hashCompile(str) | ((uint32)1 << 31))
		{};

		operator uint32() const
		{
			return value;
		}
	};
}

#endif // guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
