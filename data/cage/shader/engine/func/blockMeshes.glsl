
struct meshStruct
{
	mat4 mvpMat;
	mat4 mvpPrevMat;
	mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
	mat3x4 mMat;
	vec4 color; // color rgb is linear, and NOT alpha-premultiplied
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
