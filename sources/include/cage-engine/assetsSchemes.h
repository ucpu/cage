#ifndef guard_assetsSchemes_dsrtzh54kd
#define guard_assetsSchemes_dsrtzh54kd

#include <cage-engine/core.h>

namespace cage
{
	class GraphicsDevice;

	CAGE_ENGINE_API AssetsScheme genAssetSchemeShader(GraphicsDevice *device, uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexShader = 10;

	CAGE_ENGINE_API AssetsScheme genAssetSchemeTexture(GraphicsDevice *device, uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexTexture = 11;

	CAGE_ENGINE_API AssetsScheme genAssetSchemeModel(GraphicsDevice *device, uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexModel = 12;

	CAGE_ENGINE_API AssetsScheme genAssetSchemeRenderObject();
	constexpr uint32 AssetSchemeIndexRenderObject = 13;

	CAGE_ENGINE_API AssetsScheme genAssetSchemeSound();
	constexpr uint32 AssetSchemeIndexSound = 20;
}

#endif
