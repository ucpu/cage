#ifndef guard_assetContext_h_4lkjh9sd654gtsdfg
#define guard_assetContext_h_4lkjh9sd654gtsdfg

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
}

#endif // guard_assetContext_h_4lkjh9sd654gtsdfg
