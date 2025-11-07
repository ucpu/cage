#ifndef guard_assetsSchemes_jhdsre4r
#define guard_assetsSchemes_jhdsre4r

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API AssetPack
	{};
	CAGE_CORE_API AssetsScheme genAssetSchemePack();
	constexpr uint32 AssetSchemeIndexPack = 0;

	CAGE_CORE_API AssetsScheme genAssetSchemeRaw();
	constexpr uint32 AssetSchemeIndexRaw = 1;

	CAGE_CORE_API AssetsScheme genAssetSchemeTexts();
	constexpr uint32 AssetSchemeIndexTexts = 2;

	CAGE_CORE_API AssetsScheme genAssetSchemeCollider();
	constexpr uint32 AssetSchemeIndexCollider = 3;

	CAGE_CORE_API AssetsScheme genAssetSchemeSkeletonRig();
	constexpr uint32 AssetSchemeIndexSkeletonRig = 5;

	CAGE_CORE_API AssetsScheme genAssetSchemeSkeletalAnimation();
	constexpr uint32 AssetSchemeIndexSkeletalAnimation = 6;
}

#endif
