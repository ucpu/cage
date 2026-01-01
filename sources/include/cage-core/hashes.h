#ifndef guard_hashes_h_41hbvw8e45rfgth
#define guard_hashes_h_41hbvw8e45rfgth

#include <array>

#include <cage-core/core.h>

namespace cage
{
	CAGE_CORE_API std::array<uint8, 20> hashSha1(PointerRange<const char> data);
	CAGE_CORE_API std::array<uint8, 32> hashSha2_256(PointerRange<const char> data);
	CAGE_CORE_API std::array<uint8, 64> hashSha2_512(PointerRange<const char> data);
	CAGE_CORE_API std::array<uint8, 32> hashSha3_256(PointerRange<const char> data);
	CAGE_CORE_API std::array<uint8, 64> hashSha3_512(PointerRange<const char> data);

	CAGE_CORE_API String hashToHexadecimal(PointerRange<const uint8> data);
	CAGE_CORE_API void hexadecimalToHash(const String &inputHexadecimal, PointerRange<uint8> outputHash);

	CAGE_CORE_API String hashToBase64(PointerRange<const uint8> data);
}

#endif // guard_hashes_h_41hbvw8e45rfgth
