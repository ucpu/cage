#ifndef guard_identifier_h_cc82a428_1147_4841_aa4c_4b7784111e63_
#define guard_identifier_h_cc82a428_1147_4841_aa4c_4b7784111e63_

namespace cage
{
	namespace privat
	{
		CAGE_API void generateRandomData(uint8 *target, uint32 size);
		CAGE_API string identifierToString(const uint8 *data, uint32 size);
	}

	template<uint32 N> struct identifierStruct
	{
		explicit identifierStruct(bool randomize = false)
		{
			if (randomize)
				privat::generateRandomData(data, N);
			else
				detail::memset(data, 0, N);
		}

		uint8 data[N];

#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (const identifierStruct &other) const { return detail::memcmp(data, other.data, N) OPERATOR 0; };
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

		operator string() const
		{
			return privat::identifierToString(data, N);
		}
	};
}

#endif // guard_identifier_h_cc82a428_1147_4841_aa4c_4b7784111e63_