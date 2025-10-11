#ifndef guard_assetHeader_h_k1i7178asdvujhz89jhg14j
#define guard_assetHeader_h_k1i7178asdvujhz89jhg14j

#include <array>

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API AssetHeader
	{
		// this is the first header found in every asset file

		std::array<char, 8> cageName = { "cageAss" };
		uint32 version = 0;
		uint32 flags = 0;
		std::array<char, 128> textId = {};
		uint64 compressedSize = 0;
		uint64 originalSize = 0;
		uint16 scheme = m;
		uint16 dependenciesCount = 0;

		AssetHeader() = default;
		explicit AssetHeader(const String &textId, uint16 schemeIndex);

		// follows:
		// array of dependency names, each uint32
		// any other asset-specific data
	};
}

#endif // guard_assetHeader_h_k1i7178asdvujhz89jhg14j
