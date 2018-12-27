
$define shader vertex
$include includes.glsl
$include blockMeshes.glsl

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
layout(location = CAGE_SHADER_ATTRIB_IN_NORMAL) in vec3 inNormal;
layout(location = CAGE_SHADER_ATTRIB_IN_TANGENT) in vec3 inTangent;
layout(location = CAGE_SHADER_ATTRIB_IN_BITANGENT) in vec3 inBitangent;
layout(location = CAGE_SHADER_ATTRIB_IN_BONEINDEX) in uvec4 inBoneIndex;
layout(location = CAGE_SHADER_ATTRIB_IN_BONEWEIGHT) in vec4 inBoneWeight;

$include skeleton.glsl

out vec2 varUv;
out vec3 varNormal; // object space
out vec3 varTangent; // object space
out vec3 varBitangent; // object space
out vec3 varPosition; // world space
out vec4 varPosition4;
out vec4 varPosition4Prev;
flat out int varInstanceId;

void main()
{
$if translucent = 1
	meshIndex = 0;
$else
	meshIndex = gl_InstanceID;
$end
	pos = vec4(inPosition, 1.0);
	normal = inNormal;
	uniSkeleton();
	varUv = inUv;
	varNormal = normal;
	varTangent = inTangent;
	varBitangent = inBitangent;
	varInstanceId = gl_InstanceID;
	varPosition4 = uniMeshes[meshIndex].mvpMat * pos;
	varPosition4Prev = uniMeshes[meshIndex].mvpPrevMat * pos;
	varPosition = varPosition4.xyz / varPosition4.w;
	gl_Position = varPosition4;
}
