
/*
$define allowParsingHash true
*/

#define CAGE_SHADER_MAX_CHARACTERS 512
#define CAGE_SHADER_MAX_MESHES 85
#define CAGE_SHADER_MAX_LIGHTS 204
#define CAGE_SHADER_MAX_BONES 341
#define CAGE_SHADER_MAX_ROUTINES 16
#define CAGE_SHADER_BLOOM_DOWNSCALE 3
#define CAGE_SHADER_DOF_DOWNSCALE 3
#define CAGE_SHADER_LUMINANCE_DOWNSCALE 4

// attribute in locations

#define CAGE_SHADER_ATTRIB_IN_POSITION 0
#define CAGE_SHADER_ATTRIB_IN_NORMAL 1
#define CAGE_SHADER_ATTRIB_IN_TANGENT 2
#define CAGE_SHADER_ATTRIB_IN_BONEINDEX 3
#define CAGE_SHADER_ATTRIB_IN_BONEWEIGHT 4
#define CAGE_SHADER_ATTRIB_IN_UV 5
#define CAGE_SHADER_ATTRIB_IN_AUX0 6

// texture sampler bindings

#define CAGE_SHADER_TEXTURE_ALBEDO 0
#define CAGE_SHADER_TEXTURE_SPECIAL 1
#define CAGE_SHADER_TEXTURE_NORMAL 2
#define CAGE_SHADER_TEXTURE_USER 3
#define CAGE_SHADER_TEXTURE_ALBEDO_ARRAY 4
#define CAGE_SHADER_TEXTURE_SPECIAL_ARRAY 5
#define CAGE_SHADER_TEXTURE_NORMAL_ARRAY 6
#define CAGE_SHADER_TEXTURE_USER_ARRAY 7
#define CAGE_SHADER_TEXTURE_ALBEDO_CUBE 8
#define CAGE_SHADER_TEXTURE_SPECIAL_CUBE 9
#define CAGE_SHADER_TEXTURE_NORMAL_CUBE 10
#define CAGE_SHADER_TEXTURE_USER_CUBE 11
#define CAGE_SHADER_TEXTURE_SSAO 12
#define CAGE_SHADER_TEXTURE_COLOR 4
#define CAGE_SHADER_TEXTURE_DEPTH 5
#define CAGE_SHADER_TEXTURE_EFFECTS 6
#define CAGE_SHADER_TEXTURE_SHADOW 14
#define CAGE_SHADER_TEXTURE_SHADOW_CUBE 15

// uniform locations

#define CAGE_SHADER_UNI_SHADOWMATRIX 4
#define CAGE_SHADER_UNI_ROUTINES 8
#define CAGE_SHADER_UNI_BONESPERINSTANCE 0
#define CAGE_SHADER_UNI_LIGHTSCOUNT 1

// uniform block bindings

#define CAGE_SHADER_UNIBLOCK_VIEWPORT 0
#define CAGE_SHADER_UNIBLOCK_MATERIAL 1
#define CAGE_SHADER_UNIBLOCK_MESHES 2
#define CAGE_SHADER_UNIBLOCK_ARMATURES 3
#define CAGE_SHADER_UNIBLOCK_LIGHTS 4
#define CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES 5
#define CAGE_SHADER_UNIBLOCK_SSAO_POINTS 6

// subroutine procedure indexes

#define CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D 1
#define CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY 2
#define CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE 3
#define CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D 4
#define CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY 5
#define CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE 6
#define CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D 7
#define CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY 8
#define CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE 9
#define CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL 11
#define CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW 12
#define CAGE_SHADER_ROUTINEPROC_LIGHTSPOT 13
#define CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW 14
#define CAGE_SHADER_ROUTINEPROC_LIGHTPOINT 15
#define CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW 16

// subroutine uniform locations

#define CAGE_SHADER_ROUTINEUNIF_SKELETON 0
#define CAGE_SHADER_ROUTINEUNIF_AMBIENTLIGHTING 1
#define CAGE_SHADER_ROUTINEUNIF_AMBIENTOCCLUSION 2
#define CAGE_SHADER_ROUTINEUNIF_MAPALBEDO 3
#define CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL 4
#define CAGE_SHADER_ROUTINEUNIF_MAPNORMAL 5

/*
$undef allowParsingHash
*/
