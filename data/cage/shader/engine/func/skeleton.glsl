
layout(std140, binding = CAGE_SHADER_UNIBLOCK_ARMATURES) uniform Armatures
{
	mat3x4 uniArmatures[CAGE_SHADER_MAX_BONES];
};

layout(location = CAGE_SHADER_UNI_BONESPERINSTANCE) uniform uint uniBonesPerInstance;

void skeletonNothing()
{}

void skeletonAnimation()
{
	mat3x4 sum = mat3x4(0.0);
	for (int i = 0; i < 4; i++)
		sum += uniArmatures[meshIndex * uniBonesPerInstance + inBoneIndex[i]] * inBoneWeight[i];
	pos = transpose(mat4(sum)) * pos;
	normal = transpose(mat3(sum)) * normal;
}

void skeleton()
{
	switch (uniRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON])
	{
	default:
	case CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING: skeletonNothing(); break;
	case CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION: skeletonAnimation(); break;
	}
}
