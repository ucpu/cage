namespace cage
{
	class CAGE_API skeletonClass : private immovable
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void allocate(const mat4 &globalInverse, uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices);

		uint32 bonesCount() const;
		void animateSkin(const animationClass *animation, real coef, mat4 *temporary, mat4 *output) const;
		void animateSkeleton(const animationClass *animation, real coef, mat4 *temporary, mat4 *output) const;
	};

	CAGE_API holder<skeletonClass> newSkeleton();

	CAGE_API assetSchemeStruct genAssetSchemeSkeleton(uint32 threadIndex);
	static const uint32 assetSchemeIndexSkeleton = 13;
}
