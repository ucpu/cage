
layout(std140, binding = CAGE_SHADER_UNIBLOCK_VIEWPORT) uniform Viewport
{
	mat4 uniVpInv; // viewProj inverse
	vec4 uniEyePos;
	vec4 uniEyeDir;
	vec4 uniViewport; // x, y, w, h
	vec4 uniAmbientLight; // color rgb is linear, no alpha
	vec4 uniAmbientDirectionalLight; // color rgb is linear, no alpha
};

layout(location = CAGE_SHADER_UNI_ROUTINES) uniform uint uniRoutines[CAGE_SHADER_MAX_ROUTINES];

struct InstanceStruct
{
	mat4 mvpMat;
	mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
	mat3x4 mMat;
	vec4 color; // color rgb is linear, and NOT alpha-premultiplied
	vec4 aniTexFrames;
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MESHES) uniform Instances
{
	InstanceStruct uniInstances[CAGE_SHADER_MAX_INSTANCES];
};

vec4 pos;
vec3 position;
vec3 tangent;
vec3 bitangent;
vec3 normal;
vec3 uv;
vec3 albedo;
float opacity;
float roughness;
float metalness;
float emissive;
