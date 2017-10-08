namespace cage
{
	namespace detail
	{
		template struct CAGE_API stringBase<32>;
	}
	template struct CAGE_API delegate<void(const assetContextStruct *, void *)>;
	typedef delegate<void(const assetContextStruct *, void *)> assetDelegate;

	struct CAGE_API assetContextStruct
	{
		assetContextStruct();
		detail::stringBase<32> textName;
		mutable holdev assetHolder;
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
}
