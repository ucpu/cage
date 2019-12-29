namespace cage
{
	class CAGE_API SkeletonRig : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void allocate(const mat4 &globalInverse, uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices);

		uint32 bonesCount() const;
		void animateSkin(const SkeletalAnimation *animation, real coef, mat4 *temporary, mat4 *output) const;
		void animateSkeleton(const SkeletalAnimation *animation, real coef, mat4 *temporary, mat4 *output) const;
	};

	CAGE_API Holder<SkeletonRig> newSkeletonRig();

	CAGE_API AssetScheme genAssetSchemeSkeletonRig(uint32 threadIndex);
	static const uint32 assetSchemeIndexSkeletonRig = 13;
}
