
$include sampleTextureAnimation.glsl
$include blockMeshes.glsl

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MATERIAL) uniform Material
{
	vec4 albedoBase;
	vec4 specialBase;
	vec4 albedoMult;
	vec4 specialMult;
} uniMaterial;

layout(binding = CAGE_SHADER_TEXTURE_ALBEDO) uniform sampler2D texMaterialAlbedo2d;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO_ARRAY) uniform sampler2DArray texMaterialAlbedoArray;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO_CUBE) uniform samplerCube texMaterialAlbedoCube;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL) uniform sampler2D texMaterialSpecial2d;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL_ARRAY) uniform sampler2DArray texMaterialSpecialArray;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL_CUBE) uniform samplerCube texMaterialSpecialCube;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texMaterialNormal2d;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL_ARRAY) uniform sampler2DArray texMaterialNormalArray;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL_CUBE) uniform samplerCube texMaterialNormalCube;

subroutine void materialFunc();

layout(index = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING) subroutine (materialFunc) void materialNothing()
{}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D) subroutine (materialFunc) void materialMapAlbedo2d()
{
	vec4 a = texture(texMaterialAlbedo2d, uv.xy) * uniMaterial.albedoMult;
	albedo += a.rgb;
	opacity = a.a;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY) subroutine (materialFunc) void materialMapAlbedoArray()
{
	vec4 a = sampleTextureAnimation(texMaterialAlbedoArray, uv.xy, uniMeshes[meshIndex].aniTexFrames) * uniMaterial.albedoMult;
	albedo += a.rgb;
	opacity = a.a;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE) subroutine (materialFunc) void materialMapAlbedoCube()
{
	vec4 a = texture(texMaterialAlbedoCube, uv) * uniMaterial.albedoMult;
	albedo += a.rgb;
	opacity = a.a;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D) subroutine (materialFunc) void materialMapSpecial2d()
{
	vec4 s = texture(texMaterialSpecial2d, uv.xy) * uniMaterial.specialMult;
	roughness += s.r;
	metalness += s.g;
	emissive += s.b;
	colorMask += 1.0 - s.a;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY) subroutine (materialFunc) void materialMapSpecialArray()
{
	vec4 s = sampleTextureAnimation(texMaterialSpecialArray, uv.xy, uniMeshes[meshIndex].aniTexFrames) * uniMaterial.specialMult;
	roughness += s.r;
	metalness += s.g;
	emissive += s.b;
	colorMask += 1.0 - s.a;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE) subroutine (materialFunc) void materialMapSpecialCube()
{
	vec4 s = texture(texMaterialSpecialCube, uv) * uniMaterial.specialMult;
	roughness += s.r;
	metalness += s.g;
	emissive += s.b;
	colorMask += 1.0 - s.a;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D) subroutine (materialFunc) void materialMapNormal2d()
{
	vec3 norm = texture(texMaterialNormal2d, uv.xy).xyz * 2.0 - 1.0;
	normal = mat3(tangent, bitangent, normal) * norm;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY) subroutine (materialFunc) void materialMapNormalArray()
{
	vec3 norm = sampleTextureAnimation(texMaterialNormalArray, uv.xy, uniMeshes[meshIndex].aniTexFrames).xyz * 2.0 - 1.0;
	normal = mat3(tangent, bitangent, normal) * norm;
}

layout(index = CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE) subroutine (materialFunc) void materialMapNormalCube()
{
	vec3 norm = texture(texMaterialNormalCube, uv).xyz * 2.0 - 1.0;
	normal = mat3(tangent, bitangent, normal) * norm;
}

layout(location = CAGE_SHADER_ROUTINEUNIF_MAPALBEDO) subroutine uniform materialFunc uniMatMapAlbedo;
layout(location = CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL) subroutine uniform materialFunc uniMatMapSpecial;
layout(location = CAGE_SHADER_ROUTINEUNIF_MAPNORMAL) subroutine uniform materialFunc uniMatMapNormal;

void materialLoad()
{
	albedo = uniMaterial.albedoBase.rgb;
	opacity = uniMaterial.albedoBase.a;
	roughness = uniMaterial.specialBase.r;
	metalness = uniMaterial.specialBase.g;
	emissive = uniMaterial.specialBase.b;
	colorMask = uniMaterial.specialBase.a;
	uniMatMapAlbedo();
	uniMatMapSpecial();
	uniMatMapNormal();
	albedo = mix(albedo, uniMeshes[meshIndex].color.rgb, colorMask);
	albedo *= uniMeshes[meshIndex].color.a; // premultiplied alpha
	opacity *= uniMeshes[meshIndex].color.a;
	if (!gl_FrontFacing)
		normal = -normal;
}

void normalToWorld()
{
	mat3x4 nm = uniMeshes[meshIndex].normalMat;
	if (nm[2][3] > 0.5) // is lighting enabled
		normal = normalize(mat3(nm) * normal);
	else
		normal = vec3(0.0);
}
