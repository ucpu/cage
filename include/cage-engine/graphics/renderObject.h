namespace cage
{
	class CAGE_API renderObject : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		// lod selection properties

		real worldSize;
		real pixelsSize;
		void setLods(uint32 lodsCount, uint32 meshesCount, const float *thresholds, const uint32 *meshIndices, const uint32 *meshNames);
		uint32 lodsCount() const;
		uint32 lodSelect(float threshold) const;
		PointerRange<const uint32> meshes(uint32 lod) const;

		// default values for rendering

		vec3 color;
		real opacity;
		real texAnimSpeed;
		real texAnimOffset;
		uint32 skelAnimName;
		real skelAnimSpeed;
		real skelAnimOffset;

		renderObject();
	};

	CAGE_API Holder<renderObject> newRenderObject();

	CAGE_API AssetScheme genAssetSchemeRenderObject(uint32 threadIndex);
	static const uint32 assetSchemeIndexRenderObject = 15;
}
