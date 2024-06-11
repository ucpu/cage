
$define shader vertex

$include ../functions/common.glsl

$include uniforms.glsl

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_NORMAL) in vec3 inNormal;
layout(location = CAGE_SHADER_ATTRIB_IN_BONEINDEX) in uvec4 inBoneIndex;
layout(location = CAGE_SHADER_ATTRIB_IN_BONEWEIGHT) in vec4 inBoneWeight;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec3 inUv;

out vec3 varPosition; // world space
out vec3 varNormal; // object space
out vec3 varUv;
flat out int varInstanceId;

void skeletalAnimation()
{
	if (getOption(CAGE_SHADER_OPTIONINDEX_SKELETON) == 0)
		return;
	int bonesPerInstance = getOption(CAGE_SHADER_OPTIONINDEX_BONESCOUNT);
	mat3x4 sum = mat3x4(0);
	for (int i = 0; i < 4; i++)
		sum += uniArmatures[varInstanceId * bonesPerInstance + inBoneIndex[i]] * inBoneWeight[i];
	varPosition = vec3(transpose(mat4(sum)) * vec4(varPosition, 1));
	mat3 s = transpose(mat3(sum));
	varNormal = s * varNormal;
}

void updateVertex()
{
	varInstanceId = gl_InstanceID;
	varPosition = inPosition;
	varNormal = inNormal;
	varUv = inUv;
	skeletalAnimation();
	gl_Position = uniMeshes[varInstanceId].mvpMat * vec4(varPosition, 1);
	varPosition = transpose(uniMeshes[varInstanceId].mMat) * vec4(varPosition, 1);
}
