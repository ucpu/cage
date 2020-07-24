#ifndef guard_assetHeader_h_k1i7178asdvujhz89jhg14j
#define guard_assetHeader_h_k1i7178asdvujhz89jhg14j

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API AssetHeader
	{
		// this is the first header found in every asset file

		char cageName[8]; // cageAss\0
		uint32 version;
		uint32 flags;
		char textName[64];
		uint64 compressedSize;
		uint64 originalSize;
		uint32 aliasName;
		uint16 scheme;
		uint16 dependenciesCount;

		// follows:
		// array of dependency names, each uint32
		// any other asset-specific data
	};

	CAGE_CORE_API AssetHeader initializeAssetHeader(const string &name, uint16 schemeIndex);
}

#endif // guard_assetHeader_h_k1i7178asdvujhz89jhg14j
