
////////////////////////////////////////////////////////////////////////////////
// set = 0
////////////////////////////////////////////////////////////////////////////////

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

layout(std140, set = 0, binding = 0) uniform ViewportBlock
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

layout(std140, set = 0, binding = 1) uniform ProjectionBlock
{
	UniProjection uniProjection;
};

struct UniLight
{
	vec4 color; // linear rgb, intensity
	vec4 position; // xyz, sortingIntensity
	vec4 direction; // xyz, sortingPriority
	vec4 attenuation; // attenuationType, minDistance, maxDistance, unused
	vec4 params; // lightType, ssaoFactor, spotAngle, spotExponent
};

layout(std430, set = 0, binding = 2) readonly buffer LightsBlock
{
	UniLight uniLights[];
};

struct UniShadowedLight
{
	UniLight light;
	mat4 shadowMat[4];
	vec4 shadowParams; // shadowmapSamplerIndex, unused, normalOffsetScale, shadowFactor
	vec4 cascadesDepths;
};

layout(std430, set = 0, binding = 3) readonly buffer ShadowedLightsBlock
{
	UniShadowedLight uniShadowedLights[];
};

layout(set = 0, binding = 4) uniform sampler2D texSsao;
layout(set = 0, binding = 6) uniform sampler2D texDepth;


layout(set = 0, binding = 15) uniform sampler samplerShadows;

layout(set = 0, binding = 16) uniform texture2DArray texShadows2d_0;
layout(set = 0, binding = 17) uniform texture2DArray texShadows2d_1;
layout(set = 0, binding = 18) uniform texture2DArray texShadows2d_2;
layout(set = 0, binding = 19) uniform texture2DArray texShadows2d_3;
layout(set = 0, binding = 20) uniform texture2DArray texShadows2d_4;
layout(set = 0, binding = 21) uniform texture2DArray texShadows2d_5;
layout(set = 0, binding = 22) uniform texture2DArray texShadows2d_6;
layout(set = 0, binding = 23) uniform texture2DArray texShadows2d_7;

layout(set = 0, binding = 24) uniform textureCube texShadowsCube_0;
layout(set = 0, binding = 25) uniform textureCube texShadowsCube_1;
layout(set = 0, binding = 26) uniform textureCube texShadowsCube_2;
layout(set = 0, binding = 27) uniform textureCube texShadowsCube_3;
layout(set = 0, binding = 28) uniform textureCube texShadowsCube_4;
layout(set = 0, binding = 29) uniform textureCube texShadowsCube_5;
layout(set = 0, binding = 30) uniform textureCube texShadowsCube_6;
layout(set = 0, binding = 31) uniform textureCube texShadowsCube_7;

////////////////////////////////////////////////////////////////////////////////
// set = 1
////////////////////////////////////////////////////////////////////////////////

struct UniMaterial
{
	vec4 albedoBase; // linear rgb (NOT alpha-premultiplied), opacity
	vec4 specialBase; // roughness, metallic, emission, mask
	vec4 albedoMult;
	vec4 specialMult;
	vec4 lighting; // lighting enabled, fading transparency, unused, unused
	vec4 animation; // animation duration, animation loops, unused, unused
};

layout(std140, set = 1, binding = 0) uniform MaterialBlock
{
	UniMaterial uniMaterial;
};

#if (defined(MaterialTexRegular) && defined(MaterialTexArray)) || (defined(MaterialTexRegular) && defined(MaterialTexCube)) || (defined(MaterialTexArray) && defined(MaterialTexCube))
#error "unintended combination of keywords"
#endif

#ifdef MaterialTexRegular
layout(set = 1, binding = 1) uniform sampler2D texMaterialAlbedo;
layout(set = 1, binding = 3) uniform sampler2D texMaterialSpecial;
layout(set = 1, binding = 5) uniform sampler2D texMaterialNormal;
#endif // MaterialTexRegular

#ifdef MaterialTexArray
layout(set = 1, binding = 1) uniform sampler2DArray texMaterialAlbedo;
layout(set = 1, binding = 3) uniform sampler2DArray texMaterialSpecial;
layout(set = 1, binding = 5) uniform sampler2DArray texMaterialNormal;
#endif // MaterialTexArray

#ifdef MaterialTexCube
layout(set = 1, binding = 1) uniform samplerCube texMaterialAlbedo;
layout(set = 1, binding = 3) uniform samplerCube texMaterialSpecial;
layout(set = 1, binding = 5) uniform samplerCube texMaterialNormal;
#endif // MaterialTexCube

////////////////////////////////////////////////////////////////////////////////
// set = 2
////////////////////////////////////////////////////////////////////////////////

layout(std430, set = 2, binding = 0) readonly buffer OptionsBlock
{
	ivec4 uniOptsLights; // unshadowed lights count, shadowed lights count, enable ambient occlusion, enable normal map
	ivec4 uniOptsSkeleton; // bones count, unused, unused, unused
};

struct UniMesh
{
	mat3x4 modelMat;
	vec4 color; // linear rgb (NOT alpha-premultiplied), opacity
	vec4 animation; // time (seconds), speed, offset (normalized), unused
};

layout(std430, set = 2, binding = 1) readonly buffer MeshesBlock
{
	UniMesh uniMeshes[];
};

layout(std430, set = 2, binding = 2) readonly buffer ArmaturesBlock
{
	mat3x4 uniArmatures[];
};
