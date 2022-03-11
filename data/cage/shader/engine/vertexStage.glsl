
$define shader vertex

$include includes.glsl

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_NORMAL) in vec3 inNormal;
layout(location = CAGE_SHADER_ATTRIB_IN_TANGENT) in vec3 inTangent;
layout(location = CAGE_SHADER_ATTRIB_IN_BITANGENT) in vec3 inBitangent;
layout(location = CAGE_SHADER_ATTRIB_IN_BONEINDEX) in uvec4 inBoneIndex;
layout(location = CAGE_SHADER_ATTRIB_IN_BONEWEIGHT) in vec4 inBoneWeight;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec3 inUv;

layout(std140, binding = CAGE_SHADER_UNIBLOCK_ARMATURES) uniform Armatures
{
	mat3x4 uniArmatures[CAGE_SHADER_MAX_BONES];
};

layout(location = CAGE_SHADER_UNI_BONESPERINSTANCE) uniform uint uniBonesPerInstance;

out vec3 varUv;
out vec3 varNormal; // object space
out vec3 varTangent; // object space
out vec3 varBitangent; // object space
out vec3 varPosition; // world space
out vec4 varPosition4;
out vec4 varPosition4Prev;
flat out int varInstanceId;

void skeletonNothing()
{}

void skeletonAnimation()
{
	mat3x4 sum = mat3x4(0.0);
	for (int i = 0; i < 4; i++)
		sum += uniArmatures[varInstanceId * uniBonesPerInstance + inBoneIndex[i]] * inBoneWeight[i];
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

void main()
{
	varInstanceId = gl_InstanceID;
	pos = vec4(inPosition, 1.0);
	normal = inNormal;
	skeleton();
	varUv = inUv;
	varNormal = normal;
	varTangent = inTangent;
	varBitangent = inBitangent;
	varPosition = transpose(uniInstances[varInstanceId].mMat) * pos;
	varPosition4 = uniInstances[varInstanceId].mvpMat * pos;
	varPosition4Prev = uniInstances[varInstanceId].mvpPrevMat * pos;
	gl_Position = varPosition4;
}
