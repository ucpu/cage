
struct armatureStruct
{
	mat4 armature[CAGE_SHADER_MAX_ARMATURE_MATRICES];
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_ARMATURES) uniform Armatures
{
$if translucent = 1
	armatureStruct uniArmatures[1];
$else
	armatureStruct uniArmatures[CAGE_SHADER_MAX_RENDER_INSTANCES];
$end
};

subroutine void skeletonFunc();

layout(index = CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING) subroutine (skeletonFunc) void skeletonNothing()
{}

layout(index = CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION) subroutine (skeletonFunc) void skeletonAnimation()
{
	mat4 sum = mat4(0.0);
	for (int i = 0; i < 4; i++)
		sum += uniArmatures[meshIndex].armature[inBoneIndex[i]] * inBoneWeight[i];
	pos = sum * pos;
	normal = mat3(sum) * normal;
}

layout(location = CAGE_SHADER_ROUTINEUNIF_SKELETON) subroutine uniform skeletonFunc uniSkeleton;
