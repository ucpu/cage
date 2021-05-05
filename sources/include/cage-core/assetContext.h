#ifndef guard_assetContext_h_4lkjh9sd654gtsdfg
#define guard_assetContext_h_4lkjh9sd654gtsdfg

#include "core.h"

namespace cage
{
	typedef Delegate<void(AssetContext *)> AssetDelegate;

	struct CAGE_CORE_API AssetContext : private Immovable
	{
		const detail::StringBase<64> &textName;
		MemoryBuffer &compressedData;
		MemoryBuffer &originalData;
		Holder<void> &assetHolder;
		const uint32 realName = 0;

		explicit AssetContext(const detail::StringBase<64> &textName, MemoryBuffer &compressedData, MemoryBuffer &originalData, Holder<void> &assetHolder, uint32 realName);
	};

	struct CAGE_CORE_API AssetScheme
	{
		AssetDelegate decompress;
		AssetDelegate load;
		uint32 threadIndex = m;
		uint32 typeIndex = m;

		AssetScheme();
	};
}

#endif // guard_assetContext_h_4lkjh9sd654gtsdfg
