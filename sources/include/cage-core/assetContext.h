#ifndef guard_assetContext_h_4lkjh9sd654gtsdfg
#define guard_assetContext_h_4lkjh9sd654gtsdfg

#include "core.h"

namespace cage
{
	using AssetDelegate = Delegate<void(AssetContext *)>;

	struct CAGE_CORE_API AssetContext : private Immovable
	{
		const detail::StringBase<64> &textName;
		PointerRange<char> compressedData;
		PointerRange<char> originalData;
		Holder<void> &assetHolder;
		const uint32 realName = 0;

		explicit AssetContext(const detail::StringBase<64> &textName, PointerRange<char> compressedData, PointerRange<char> originalData, Holder<void> &assetHolder, uint32 realName);
	};

	struct CAGE_CORE_API AssetScheme
	{
		AssetDelegate decompress;
		AssetDelegate load;
		uint32 threadIndex = m;
		uint32 typeHash = m;

		AssetScheme();
	};
}

#endif // guard_assetContext_h_4lkjh9sd654gtsdfg
