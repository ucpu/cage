#ifndef guard_memoryCompression_h_edrz4gh6ret54zh6r4t
#define guard_memoryCompression_h_edrz4gh6ret54zh6r4t

#include "core.h"

namespace cage
{
	// preference = 100 -> best compression ratio, but very slow
	// preference = 0 -> full compression speed, but worse compression ratio
	CAGE_CORE_API Holder<PointerRange<char>> compress(PointerRange<const char> input, sint32 preference = 90);
	CAGE_CORE_API Holder<PointerRange<char>> decompress(PointerRange<const char> input, uintPtr outputSize);
	CAGE_CORE_API void compress(PointerRange<const char> input, PointerRange<char> &output, sint32 preference = 90);
	CAGE_CORE_API void decompress(PointerRange<const char> input, PointerRange<char> &output);
	CAGE_CORE_API uintPtr compressionBound(uintPtr size);
}

#endif // guard_memoryCompression_h_edrz4gh6ret54zh6r4t
