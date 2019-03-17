namespace cage
{
	typedef delegate<void(const assetContextStruct *, void *)> assetDelegate;

	struct CAGE_API assetContextStruct
	{
		assetContextStruct();
		detail::stringBase<64> textName;
		mutable holder<void> assetHolder;
		uint64 compressedSize;
		uint64 originalSize;
		mutable void *assetPointer;
		mutable void *returnData;
		void *compressedData;
		void *originalData;
		uint32 realName;
		uint32 internationalizedName;
		uint32 assetFlags;
	};

	struct CAGE_API assetSchemeStruct
	{
		assetSchemeStruct();
		assetDelegate decompress;
		assetDelegate load;
		assetDelegate done;
		void *schemePointer;
		uint32 threadIndex;
	};

	struct CAGE_API assetHeaderStruct
	{
		// this is the first header found in every asset file

		char cageName[8]; // cageAss\0
		uint32 version;
		uint32 flags;
		char textName[64];
		uint64 compressedSize;
		uint64 originalSize;
		uint32 internationalizedName;
		uint16 scheme;
		uint16 dependenciesCount;

		// follows:
		// array of dependency names, each uint32
		// any other asset-specific data
	};

	/*
	struct CAGE_API textPackHeaderStruct
	{
		// follows:
		// count, uint32, number of texts
		// name, uint32
		// length, uint32
		// array of chars
		// name, uint32
		// ...
	};

	struct CAGE_API colliderHeaderStruct
	{
		// follows:
		// serialized collider data (possibly compressed)
	};
	*/
}
