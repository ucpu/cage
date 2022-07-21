#ifndef guard_hashes_h_41hbvw8e45rfgth
#define guard_hashes_h_41hbvw8e45rfgth

#include "core.h"

#include <array>

namespace cage
{
	CAGE_CORE_API std::array<uint8, 20> hashSha1(PointerRange<const char> data);

	CAGE_CORE_API String hashToHexadecimal(PointerRange<const uint8> data);
	CAGE_CORE_API String hashToBase64(PointerRange<const uint8> data);
}

#endif // guard_hashes_h_41hbvw8e45rfgth
