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

	CAGE_API assetSchemeStruct genAssetSchemeShader(uint32 threadIndex, windowClass *memoryContext);
	CAGE_API assetSchemeStruct genAssetSchemeTexture(uint32 threadIndex, windowClass *memoryContext);
	CAGE_API assetSchemeStruct genAssetSchemeMesh(uint32 threadIndex, windowClass *memoryContext);
	CAGE_API assetSchemeStruct genAssetSchemeSkeleton(uint32 threadIndex);
	CAGE_API assetSchemeStruct genAssetSchemeAnimation(uint32 threadIndex);
	CAGE_API assetSchemeStruct genAssetSchemeObject(uint32 threadIndex);
	CAGE_API assetSchemeStruct genAssetSchemeFont(uint32 threadIndex, windowClass *memoryContext);
	static const uint32 assetSchemeIndexShader = 10;
	static const uint32 assetSchemeIndexTexture = 11;
	static const uint32 assetSchemeIndexMesh = 12;
	static const uint32 assetSchemeIndexSkeleton = 13;
	static const uint32 assetSchemeIndexAnimation = 14;
	static const uint32 assetSchemeIndexObject = 15;
	static const uint32 assetSchemeIndexFont = 16;

	CAGE_API assetSchemeStruct genAssetSchemeSound(uint32 threadIndex, soundContextClass *memoryContext);
	static const uint32 assetSchemeIndexSound = 20;
}
