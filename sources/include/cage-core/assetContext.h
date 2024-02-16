#ifndef guard_assetContext_h_4lkjh9sd654gtsdfg
#define guard_assetContext_h_4lkjh9sd654gtsdfg

#include <cage-core/typeIndex.h>

namespace cage
{
	struct AssetContext;

	using AssetDelegate = Delegate<void(AssetContext *)>;

	struct CAGE_CORE_API AssetContext : private Immovable
	{
		detail::StringBase<64> textName;
		Holder<void> customData;
		Holder<PointerRange<uint32>> dependencies;
		Holder<PointerRange<char>> compressedData;
		Holder<PointerRange<char>> originalData;
		Holder<void> assetHolder;
		const uint32 realName = 0;
		uint32 aliasName = 0;
		uint32 scheme = m;

		AssetContext(uint32 realName) : realName(realName) {}
	};

	struct CAGE_CORE_API AssetScheme
	{
		AssetDelegate fetch;
		AssetDelegate decompress;
		AssetDelegate load;
		uint32 threadIndex = m;
		uint32 typeHash = m;

		AssetScheme();
	};
}

#endif // guard_assetContext_h_4lkjh9sd654gtsdfg
