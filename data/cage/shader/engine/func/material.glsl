
$include sampleTextureAnimation.glsl
$include blockMeshes.glsl

layout(std140, binding = CAGE_SHADER_UNIBLOCK_MATERIAL) uniform Material
{
	// albedo rgb is linear, and NOT alpha-premultiplied
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

vec4 materialNothing()
{
	return vec4(0.0, 0.0, 0.0, 1.0);
}

vec4 materialMapAlbedo2d()
{
	return texture(texMaterialAlbedo2d, uv.xy);
}

vec4 materialMapAlbedoArray()
{
	return sampleTextureAnimation(texMaterialAlbedoArray, uv.xy, uniMeshes[meshIndex].aniTexFrames);
}

vec4 materialMapAlbedoCube()
{
	return texture(texMaterialAlbedoCube, uv);
}

vec4 materialMapSpecial2d()
{
	return texture(texMaterialSpecial2d, uv.xy);
}

vec4 materialMapSpecialArray()
{
	return sampleTextureAnimation(texMaterialSpecialArray, uv.xy, uniMeshes[meshIndex].aniTexFrames);
}

vec4 materialMapSpecialCube()
{
	return texture(texMaterialSpecialCube, uv);
}

vec4 restoreNormalMapZ(vec4 n)
{
	n.z = sqrt(1 - clamp(dot(n.xy, n.xy), 0, 1));
	//n.xyz = normalize(n.xyz);
	return n;
}

vec4 materialMapNormal2d()
{
	return restoreNormalMapZ(texture(texMaterialNormal2d, uv.xy));
}

vec4 materialMapNormalArray()
{
	return restoreNormalMapZ(sampleTextureAnimation(texMaterialNormalArray, uv.xy, uniMeshes[meshIndex].aniTexFrames));
}

vec4 materialMapNormalCube()
{
	return restoreNormalMapZ(texture(texMaterialNormalCube, uv));
}

vec4 matMapImpl(int index)
{
	switch (uniRoutines[index])
	{
		case CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING: return materialNothing();
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D: return materialMapAlbedo2d();
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY: return materialMapAlbedoArray();
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE: return materialMapAlbedoCube();
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D: return materialMapSpecial2d();
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY: return materialMapSpecialArray();
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE: return materialMapSpecialCube();
		case CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D: return materialMapNormal2d();
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY: return materialMapNormalArray();
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE: return materialMapNormalCube();
	}
	return vec4(-100.0);
}

void materialLoad()
{
	vec4 specialMap = matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL);
	vec4 special4 = uniMaterial.specialBase + specialMap * uniMaterial.specialMult;
	roughness = special4.r;
	metalness = special4.g;
	emissive = special4.b;

	vec4 albedoMap = matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO);
	if (albedoMap.a > 0.0)
		albedoMap.rgb /= albedoMap.a; // depremultiply albedo texture
	vec4 albedo4 = uniMaterial.albedoBase + albedoMap * uniMaterial.albedoMult;
	albedo = albedo4.rgb;
	opacity = albedo4.a;

	albedo = mix(uniMeshes[meshIndex].color.rgb, albedo, special4.a);
	opacity *= uniMeshes[meshIndex].color.a;
	albedo *= opacity; // premultiplied alpha

	if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] != CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING)
	{
		vec4 normalMap = matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL);
		normal = mat3(tangent, bitangent, normal) * (normalMap.xyz * 2.0 - 1.0);
	}

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
