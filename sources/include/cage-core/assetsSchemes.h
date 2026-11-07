#ifndef guard_assetsSchemes_jhdsre4r
#define guard_assetsSchemes_jhdsre4r

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API AssetPack
	{};
	CAGE_CORE_API AssetsScheme genAssetSchemePack();
	constexpr SchemeIndex AssetSchemeIndexPack = SchemeIndex(0);

	CAGE_CORE_API AssetsScheme genAssetSchemeRaw();
	constexpr SchemeIndex AssetSchemeIndexRaw = SchemeIndex(1);

	CAGE_CORE_API AssetsScheme genAssetSchemeTexts();
	constexpr SchemeIndex AssetSchemeIndexTexts = SchemeIndex(2);

	CAGE_CORE_API AssetsScheme genAssetSchemeCollider();
	constexpr SchemeIndex AssetSchemeIndexCollider = SchemeIndex(3);

	CAGE_CORE_API AssetsScheme genAssetSchemeSkeletonRig();
	constexpr SchemeIndex AssetSchemeIndexSkeletonRig = SchemeIndex(5);

	CAGE_CORE_API AssetsScheme genAssetSchemeSkeletalAnimation();
	constexpr SchemeIndex AssetSchemeIndexSkeletalAnimation = SchemeIndex(6);
}

#endif
