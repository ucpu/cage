namespace cage
{
	class CAGE_API skeletonRig : private immovable
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void allocate(const mat4 &globalInverse, uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices);

		uint32 bonesCount() const;
		void animateSkin(const skeletalAnimation *animation, real coef, mat4 *temporary, mat4 *output) const;
		void animateSkeleton(const skeletalAnimation *animation, real coef, mat4 *temporary, mat4 *output) const;
	};

	CAGE_API holder<skeletonRig> newSkeletonRig();

	CAGE_API assetScheme genAssetSchemeSkeletonRig(uint32 threadIndex);
	static const uint32 assetSchemeIndexSkeletonRig = 13;
}
