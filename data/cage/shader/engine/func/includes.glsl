
layout(std140, binding = CAGE_SHADER_UNIBLOCK_VIEWPORT) uniform Viewport
{
	mat4 uniVpInv;
	vec4 uniEyePos;
	vec4 uniEyeDir;
	vec4 uniViewport;
	vec4 uniAmbientLight;
	vec4 uniAmbientDirectionalLight;
};

layout(location = CAGE_SHADER_UNI_ROUTINES) uniform uint uniRoutines[CAGE_SHADER_MAX_ROUTINES];

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
int meshIndex;
int lightIndex;
