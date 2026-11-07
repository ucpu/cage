#ifndef guard_assetsSchemes_dsrtzh54kd
#define guard_assetsSchemes_dsrtzh54kd

#include <cage-engine/core.h>

namespace cage
{
	class GraphicsDevice;

	CAGE_ENGINE_API AssetsScheme genAssetSchemeShader(GraphicsDevice *device);
	constexpr SchemeIndex AssetSchemeIndexShader = SchemeIndex(10);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeTexture(GraphicsDevice *device);
	constexpr SchemeIndex AssetSchemeIndexTexture = SchemeIndex(11);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeModel(GraphicsDevice *device);
	constexpr SchemeIndex AssetSchemeIndexModel = SchemeIndex(12);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeRenderObject();
	constexpr SchemeIndex AssetSchemeIndexRenderObject = SchemeIndex(13);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeFont();
	constexpr SchemeIndex AssetSchemeIndexFont = SchemeIndex(14);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeSound();
	constexpr SchemeIndex AssetSchemeIndexSound = SchemeIndex(20);
}

#endif
