#ifndef guard_assetsStructs_h_k1i71789ds69r787jhg14j
#define guard_assetsStructs_h_k1i71789ds69r787jhg14j

#include "core.h"

namespace cage
{
	typedef Delegate<void(const AssetContext *)> AssetDelegate;

	struct CAGE_CORE_API AssetContext : private Immovable
	{
		detail::StringBase<64> textName;
		mutable Holder<void> assetHolder;
		const uint32 realName = 0;
		uint32 aliasName = 0;
		uint32 assetFlags = 0;

		AssetContext(uint32 realName);
		MemoryBuffer &compressedData() const;
		MemoryBuffer &originalData() const;
	};

	struct CAGE_CORE_API AssetScheme
	{
		AssetDelegate decompress;
		AssetDelegate load;
		uint32 threadIndex = m;

		AssetScheme();
	};

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

#endif // guard_assetsStructs_h_k1i71789ds69r787jhg14j
