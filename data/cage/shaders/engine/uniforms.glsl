
struct UniViewport
{
	// this matrix is same for shadowmaps and for the camera
	mat4 viewMat; // camera view matrix
	vec4 eyePos;
	vec4 eyeDir;
	vec4 viewport; // x, y, w, h
	vec4 ambientLight; // linear rgb, unused
	vec4 skyLight; // linear rgb, unused
	vec4 time; // frame index (loops at 10000), time (loops every second), time (loops every 1000 seconds), unused
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_VIEWPORT) uniform ViewportBlock
{
	UniViewport uniViewport;
};

struct UniProjection
{
	// these matrices are different for each shadowmap and the actual camera
	mat4 viewMat;
	mat4 projMat;
	mat4 vpMat;
	mat4 vpInv;
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_PROJECTION) uniform ProjectionBlock
{
	UniProjection uniProjection;
};

struct UniMaterial
{
	vec4 albedoBase; // linear rgb (NOT alpha-premultiplied), opacity
	vec4 specialBase; // roughness, metallic, emission, mask
	vec4 albedoMult;
	vec4 specialMult;
	vec4 lighting; // lighting enabled, fading transparency, unused, unused
	vec4 animation; // animation duration, animation loops, unused, unused
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MATERIAL) uniform MaterialBlock
{
	UniMaterial uniMaterial;
};

struct UniMesh
{
	mat3x4 modelMat;
	vec4 color; // linear rgb (NOT alpha-premultiplied), opacity
	vec4 animation; // time (seconds), speed, offset (normalized), unused
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MESHES) uniform MeshesBlock
{
	UniMesh uniMeshes[CAGE_SHADER_MAX_MESHES];
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_ARMATURES) uniform ArmaturesBlock
{
	mat3x4 uniArmatures[CAGE_SHADER_MAX_BONES];
};

struct UniLight
{
	vec4 color; // linear rgb, intensity
	vec4 position; // xyz, sortingIntensity
	vec4 direction; // xyz, sortingPriority
	vec4 attenuation; // attenuationType, minDistance, maxDistance, unused
	vec4 params; // lightType, ssaoFactor, spotAngle, spotExponent
};

struct UniShadowedLight
{
	UniLight light;
	mat4 shadowMat[CAGE_SHADER_MAX_SHADOWMAPSCASCADES];
	vec4 shadowParams; // shadowmapSamplerIndex, unused, normalOffsetScale, shadowFactor
	vec4 cascadesDepths;
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_LIGHTS) uniform LightsBlock
{
	UniLight uniLights[CAGE_SHADER_MAX_LIGHTS];
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_SHADOWEDLIGHTS) uniform ShadowedLightsBlock
{
	UniShadowedLight uniShadowedLights[CAGE_SHADER_MAX_SHADOWMAPSCUBE + CAGE_SHADER_MAX_SHADOWMAPS2D];
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_OPTIONS) uniform OptionsBlock
{
	ivec4 uniOptions[(CAGE_SHADER_MAX_OPTIONS + 3) / 4];
};

int getOption(int index)
{
	return uniOptions[index / 4][index % 4];
}

layout(binding = CAGE_SHADER_TEXTURE_ALBEDO) uniform sampler2D texMaterialAlbedo2d;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO_ARRAY) uniform sampler2DArray texMaterialAlbedoArray;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO_CUBE) uniform samplerCube texMaterialAlbedoCube;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL) uniform sampler2D texMaterialSpecial2d;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL_ARRAY) uniform sampler2DArray texMaterialSpecialArray;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL_CUBE) uniform samplerCube texMaterialSpecialCube;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texMaterialNormal2d;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL_ARRAY) uniform sampler2DArray texMaterialNormalArray;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL_CUBE) uniform samplerCube texMaterialNormalCube;
layout(binding = CAGE_SHADER_TEXTURE_SSAO) uniform sampler2D texSsao;
layout(binding = CAGE_SHADER_TEXTURE_SHADOWMAP2D0) uniform sampler2DArray texShadows2d[CAGE_SHADER_MAX_SHADOWMAPS2D];
layout(binding = CAGE_SHADER_TEXTURE_SHADOWMAPCUBE0) uniform samplerCube texShadowsCube[CAGE_SHADER_MAX_SHADOWMAPSCUBE];
