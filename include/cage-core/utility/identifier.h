#ifndef guard_identifier_h_cc82a428_1147_4841_aa4c_4b7784111e63_
#define guard_identifier_h_cc82a428_1147_4841_aa4c_4b7784111e63_

namespace cage
{
	namespace privat
	{
		CAGE_API void generateRandomData(uint8 *target, uint32 size);
	}

	template<uint32 N> struct identifier
	{
		explicit identifier(bool randomize = false)
		{
			if (randomize)
				privat::generateRandomData(data, N);
			else
				detail::memset(data, 0, N);
		}

		uint8 data[N];

#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (const identifier &other) const { return detail::memcmp(data, other.data, N) OPERATOR 0; };
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

		operator string() const
		{
			string res;
			for (uint32 i = 0; i < N; i++)
			{
				char a = (char)(data[i] / 16) + (data[i] / 16 < 10 ? '0' : 'a' - 10);
				res += string(&a, 1);
				char b = (char)(data[i] % 16) + (data[i] % 16 < 10 ? '0' : 'a' - 10);
				res += string(&b, 1);
			}
			return res;
		}
	};
}

#endif // guard_identifier_h_cc82a428_1147_4841_aa4c_4b7784111e63_
