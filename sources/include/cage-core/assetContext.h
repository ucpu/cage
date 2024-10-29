#ifndef guard_assetContext_h_4lkjh9sd654gtsdfg
#define guard_assetContext_h_4lkjh9sd654gtsdfg

#include <cage-core/typeIndex.h>

namespace cage
{
	struct AssetContext;

	using AssetDelegate = Delegate<void(AssetContext *)>;

	struct CAGE_CORE_API AssetContext : private Immovable
	{
		detail::StringBase<64> textId;
		Holder<void> customData;
		Holder<PointerRange<uint32>> dependencies;
		Holder<PointerRange<char>> compressedData;
		Holder<PointerRange<char>> originalData;
		Holder<void> assetHolder;
		const uint32 assetId = 0;
		uint32 scheme = m;

		AssetContext(uint32 assetId) : assetId(assetId) {}
	};

	struct CAGE_CORE_API AssetsScheme
	{
		AssetDelegate fetch;
		AssetDelegate decompress;
		AssetDelegate load;
		uint32 threadIndex = m;
		uint32 typeHash = m;

		AssetsScheme();
	};
}

#endif // guard_assetContext_h_4lkjh9sd654gtsdfg
