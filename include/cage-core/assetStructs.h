#ifndef guard_assetsStructs_h_k1i71789ds69r787jhg14j
#define guard_assetsStructs_h_k1i71789ds69r787jhg14j

namespace cage
{
	typedef Delegate<void(const AssetContext *, void *)> AssetDelegate;

	struct CAGE_API AssetContext
	{
		AssetContext();
		detail::StringBase<64> textName;
		mutable Holder<void> assetHolder;
		uint64 compressedSize;
		uint64 originalSize;
		mutable void *assetPointer;
		mutable void *returnData;
		void *compressedData;
		void *originalData;
		uint32 realName;
		uint32 aliasName;
		uint32 assetFlags;
	};

	struct CAGE_API AssetScheme
	{
		AssetScheme();
		AssetDelegate decompress;
		AssetDelegate load;
		AssetDelegate done;
		void *schemePointer;
		uint32 threadIndex;
	};

	struct CAGE_API AssetHeader
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

	/*
	struct CAGE_API TextPackHeader
	{
		// follows:
		// count, uint32, number of texts
		// name, uint32
		// length, uint32
		// array of chars
		// name, uint32
		// ...
	};

	struct CAGE_API ColliderHeader
	{
		// follows:
		// serialized collider data (possibly compressed)
	};
	*/

	CAGE_API AssetHeader initializeAssetHeader(const string &name, uint16 schemeIndex);
}

#endif // guard_assetsStructs_h_k1i71789ds69r787jhg14j
