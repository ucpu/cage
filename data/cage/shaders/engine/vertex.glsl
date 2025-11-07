
$include ../functions/common.glsl

$include uniforms.glsl

layout(location = 0) varying vec3 varPosition; // world space
layout(location = 1) varying vec3 varNormal; // object space
layout(location = 2) varying vec3 varUv;
layout(location = 3) varying flat int varInstanceId;

$define shader vertex

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uvec4 inBoneIndex;
layout(location = 3) in vec4 inBoneWeight;
layout(location = 4) in vec3 inUv;

void propagateInputs()
{
	varInstanceId = gl_InstanceIndex;
	varPosition = inPosition;
#ifdef Normals
	varNormal = inNormal;
#else
	varNormal = vec3(0);
#endif // Normals
	varUv = inUv;
}

void skeletalAnimation()
{
#ifdef SkeletalAnimation
	int bonesPerInstance = uniOptsSkeleton[0];
	mat3x4 sum = mat3x4(0);
	for (int i = 0; i < 4; i++)
		sum += uniArmatures[varInstanceId * bonesPerInstance + inBoneIndex[i]] * inBoneWeight[i];
	varPosition = vec3(transpose(mat4(sum)) * vec4(varPosition, 1));
	mat3 s = transpose(mat3(sum));
	varNormal = s * varNormal;
#endif // SkeletalAnimation
}

void computePosition()
{
	gl_Position = uniProjection.vpMat * mat4(transpose(uniMeshes[varInstanceId].modelMat)) * vec4(varPosition, 1);
	varPosition = transpose(uniMeshes[varInstanceId].modelMat) * vec4(varPosition, 1);
}
