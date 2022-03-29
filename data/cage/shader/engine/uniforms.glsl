
struct UniViewport
{
	mat4 vpInv; // viewProj inverse
	vec4 eyePos;
	vec4 eyeDir;
	vec4 viewport; // x, y, w, h
	vec4 ambientLight; // color rgb is linear, no alpha
	vec4 ambientDirectionalLight; // color rgb is linear, no alpha
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_VIEWPORT) uniform ViewportBlock
{
	UniViewport uniViewport;
};

struct UniMaterial
{
	// albedo rgb is linear, and NOT alpha-premultiplied
	vec4 albedoBase;
	vec4 specialBase;
	vec4 albedoMult;
	vec4 specialMult;
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MATERIAL) uniform MaterialBlock
{
	UniMaterial uniMaterial;
};

struct UniMesh
{
	mat4 mvpMat;
	mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
	mat3x4 mMat;
	vec4 color; // color rgb is linear, and NOT alpha-premultiplied
	vec4 aniTexFrames;
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
	vec4 color;
	vec4 position;
	vec4 direction;
	vec4 attenuation;
	vec4 parameters; // spotAngle, spotExponent, normalOffsetScale, lightType
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_LIGHTS) uniform LightsBlock
{
	UniLight uniLights[CAGE_SHADER_MAX_LIGHTS];
};

layout(location = CAGE_SHADER_UNI_BONESPERINSTANCE) uniform uint uniBonesPerInstance;
layout(location = CAGE_SHADER_UNI_LIGHTSCOUNT) uniform uint uniLightsCount;
layout(location = CAGE_SHADER_UNI_SHADOWMATRIX) uniform mat4 uniShadowMatrix;
layout(location = CAGE_SHADER_UNI_ROUTINES) uniform uint uniRoutines[CAGE_SHADER_MAX_ROUTINES];

layout(binding = CAGE_SHADER_TEXTURE_ALBEDO) uniform sampler2D texMaterialAlbedo2d;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO_ARRAY) uniform sampler2DArray texMaterialAlbedoArray;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO_CUBE) uniform samplerCube texMaterialAlbedoCube;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL) uniform sampler2D texMaterialSpecial2d;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL_ARRAY) uniform sampler2DArray texMaterialSpecialArray;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL_CUBE) uniform samplerCube texMaterialSpecialCube;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texMaterialNormal2d;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL_ARRAY) uniform sampler2DArray texMaterialNormalArray;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL_CUBE) uniform samplerCube texMaterialNormalCube;
layout(binding = CAGE_SHADER_TEXTURE_SHADOW) uniform sampler2D texShadow2d;
layout(binding = CAGE_SHADER_TEXTURE_SHADOW_CUBE) uniform samplerCube texShadowCube;
layout(binding = CAGE_SHADER_TEXTURE_SSAO) uniform sampler2D texSsao;
