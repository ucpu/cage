namespace cage
{
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
}
