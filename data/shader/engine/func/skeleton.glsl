
layout(std140, binding = CAGE_SHADER_UNIBLOCK_ARMATURES) uniform Armatures
{
	mat3x4 uniArmatures[CAGE_SHADER_MAX_BONES];
};

layout(location = CAGE_SHADER_UNI_BONESPERINSTANCE) uniform uint uniBonesPerInstance;

subroutine void skeletonFunc();

layout(index = CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING) subroutine (skeletonFunc) void skeletonNothing()
{}

layout(index = CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION) subroutine (skeletonFunc) void skeletonAnimation()
{
	mat3x4 sum = mat3x4(0.0);
	for (int i = 0; i < 4; i++)
		sum += uniArmatures[meshIndex * uniBonesPerInstance + inBoneIndex[i]] * inBoneWeight[i];
	pos = transpose(mat4(sum)) * pos;
	normal = transpose(mat3(sum)) * normal;
}

layout(location = CAGE_SHADER_ROUTINEUNIF_SKELETON) subroutine uniform skeletonFunc uniSkeleton;
