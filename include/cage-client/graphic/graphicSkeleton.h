namespace cage
{
	class CAGE_API skeletonClass
	{
	public:
		void allocate(uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices, uint32 namedBones, const uint32 *namedBoneNames, const uint16 *namedBoneIndexes);

		uint32 bonesCount() const;
		uint16 findNamedBone(uint32 name) const;
		void calculatePose(const animationClass *animation, real time, mat4 *temporary, mat4 *output);
	};

	CAGE_API holder<skeletonClass> newSkeleton();
}
