namespace cage
{
	class CAGE_API objectClass : private immovable
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		real worldSize;
		real pixelsSize;

		void setLods(uint32 lodsCount, uint32 meshesCount, const float *thresholds, const uint32 *meshIndices, const uint32 *meshNames);

		uint32 lodsCount() const;
		uint32 lodSelect(float threshold) const;
		pointerRange<const uint32> meshes(uint32 lod) const;
	};

	CAGE_API holder<objectClass> newObject();

	CAGE_API assetScheme genAssetSchemeObject(uint32 threadIndex);
	static const uint32 assetSchemeIndexObject = 15;
}
