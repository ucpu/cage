namespace cage
{
	CAGE_API assetSchemeStruct genAssetSchemePack(const uint32 threadIndex);
	static const uint32 assetSchemeIndexPack = 0;

	CAGE_API assetSchemeStruct genAssetSchemeRaw(const uint32 threadIndex);
	static const uint32 assetSchemeIndexRaw = 1;

	CAGE_API assetSchemeStruct genAssetSchemeTextPackage(const uint32 threadIndex);
	static const uint32 assetSchemeIndexTextPackage = 2;

	CAGE_API assetSchemeStruct genAssetSchemeCollider(const uint32 threadIndex);
	static const uint32 assetSchemeIndexCollider = 3;
}
