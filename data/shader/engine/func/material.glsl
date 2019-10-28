
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

void materialNothing()
{}

void materialMapAlbedo2d()
{
	vec4 a = texture(texMaterialAlbedo2d, uv.xy) * uniMaterial.albedoMult;
	albedo += a.rgb;
	opacity = a.a;
}

void materialMapAlbedoArray()
{
	vec4 a = sampleTextureAnimation(texMaterialAlbedoArray, uv.xy, uniMeshes[meshIndex].aniTexFrames) * uniMaterial.albedoMult;
	albedo += a.rgb;
	opacity = a.a;
}

void materialMapAlbedoCube()
{
	vec4 a = texture(texMaterialAlbedoCube, uv) * uniMaterial.albedoMult;
	albedo += a.rgb;
	opacity = a.a;
}

void materialMapSpecial2d()
{
	vec4 s = texture(texMaterialSpecial2d, uv.xy) * uniMaterial.specialMult;
	roughness += s.r;
	metalness += s.g;
	emissive += s.b;
	colorMask += 1.0 - s.a;
}

void materialMapSpecialArray()
{
	vec4 s = sampleTextureAnimation(texMaterialSpecialArray, uv.xy, uniMeshes[meshIndex].aniTexFrames) * uniMaterial.specialMult;
	roughness += s.r;
	metalness += s.g;
	emissive += s.b;
	colorMask += 1.0 - s.a;
}

void materialMapSpecialCube()
{
	vec4 s = texture(texMaterialSpecialCube, uv) * uniMaterial.specialMult;
	roughness += s.r;
	metalness += s.g;
	emissive += s.b;
	colorMask += 1.0 - s.a;
}

void materialMapNormal2d()
{
	vec3 norm = texture(texMaterialNormal2d, uv.xy).xyz * 2.0 - 1.0;
	normal = mat3(tangent, bitangent, normal) * norm;
}

void materialMapNormalArray()
{
	vec3 norm = sampleTextureAnimation(texMaterialNormalArray, uv.xy, uniMeshes[meshIndex].aniTexFrames).xyz * 2.0 - 1.0;
	normal = mat3(tangent, bitangent, normal) * norm;
}

void materialMapNormalCube()
{
	vec3 norm = texture(texMaterialNormalCube, uv).xyz * 2.0 - 1.0;
	normal = mat3(tangent, bitangent, normal) * norm;
}

void matMapImpl(int index)
{
	switch (uniRoutines[index])
	{
		case CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING: materialNothing(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D: materialMapAlbedo2d(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY: materialMapAlbedoArray(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE: materialMapAlbedoCube(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D: materialMapSpecial2d(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY: materialMapSpecialArray(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE: materialMapSpecialCube(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D: materialMapNormal2d(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY: materialMapNormalArray(); break;
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE: materialMapNormalCube(); break;
	}
}

void matMapAlbedo()
{
	matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO);
}

void matMapSpecial()
{
	matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL);
}

void matMapNormal()
{
	matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL);
}

void materialLoad()
{
	albedo = uniMaterial.albedoBase.rgb;
	opacity = uniMaterial.albedoBase.a;
	roughness = uniMaterial.specialBase.r;
	metalness = uniMaterial.specialBase.g;
	emissive = uniMaterial.specialBase.b;
	colorMask = uniMaterial.specialBase.a;
	matMapAlbedo();
	matMapSpecial();
	matMapNormal();
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
