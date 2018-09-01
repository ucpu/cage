#ifndef guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
#define guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_

#define GCHL_HASH_OFFSET 2166136261u
#define GCHL_HASH_PRIME 16777619u

namespace cage
{
	namespace privat
	{
		CAGE_API const uint32 hashStringFunction(const char *str);
	}

	struct CAGE_API hashString
	{
	private:
		const uint32 value;

		struct constCharWrapper
		{
			constCharWrapper(const char *str) : str(str) {};
			const char *str;
		};

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
		explicit hashString(const constCharWrapper &str) : value(privat::hashStringFunction(str.str))
		{};

		template<uint32 N>
		explicit hashString(const char(&str)[N]) : value(hash<N, N>()(str))
		{};

		operator uint32() const
		{
			return value;
		}
	};
}

#endif // guard_hashString_h_b051260b_de9f_4127_a5c7_1c6a0d0b6798_
