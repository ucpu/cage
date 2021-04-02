#ifndef guard_quid_h_cc82a428_1147_4841_aa4c_4b7784111e63_
#define guard_quid_h_cc82a428_1147_4841_aa4c_4b7784111e63_

#include "string.h"

namespace cage
{
	namespace privat
	{
		CAGE_CORE_API void generateRandomData(uint8 *target, uint32 size);
		CAGE_CORE_API string guidToString(const uint8 *data, uint32 size);
	}

	template<uint32 N>
	struct Guid
	{
		explicit Guid(bool randomize = false)
		{
			if (randomize)
				privat::generateRandomData(data, N);
			else
				detail::memset(data, 0, N);
		}

		uint8 data[N];

		operator string() const
		{
			return privat::guidToString(data, N);
		}

		auto operator <=> (const Guid &) const noexcept = default;
		bool operator == (const Guid &) const noexcept = default;
	};
}

#endif // guard_quid_h_cc82a428_1147_4841_aa4c_4b7784111e63_
