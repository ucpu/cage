
struct meshStruct
{
	mat4 mvpMat;
	mat4 mvpPrevMat;
	mat3x4 normalMat;
	mat3x4 mMat;
	vec4 color;
	vec4 aniTexFrames;
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MESHES) uniform Meshes
{
$if translucent = 1
	meshStruct uniMeshes[1];
$else
	meshStruct uniMeshes[CAGE_SHADER_MAX_INSTANCES];
$end
};