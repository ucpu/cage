namespace cage
{
	class CAGE_API skeletonClass
	{
	public:
		void allocate(const mat4 &globalInverse, uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices);

		uint32 bonesCount() const;
		void animateSkin(const animationClass *animation, real coef, mat4 *temporary, mat4 *output) const;
		void animateSkeleton(const animationClass *animation, real coef, mat4 *temporary, mat4 *output) const;
	};

	CAGE_API holder<skeletonClass> newSkeleton();
}
