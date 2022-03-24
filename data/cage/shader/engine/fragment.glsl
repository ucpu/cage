
$define shader fragment

$include ../functions/brdf.glsl
$include ../functions/attenuation.glsl
$include ../functions/sampleShadowMap.glsl
$include ../functions/sampleTextureAnimation.glsl

$include uniforms.glsl

in vec3 varPosition; // world space
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varBitangent; // object space
in vec3 varUv;
flat in int varInstanceId;

vec3 normal; // world space

struct Material
{
	vec3 albedo; // linear
	float opacity;
	float roughness;
	float metalness;
	float emissive;
};

vec3 lightingBrdf(Material material, vec3 L, vec3 V)
{
	return brdf(normal, L, V, material.albedo, material.roughness, material.metalness);
}

vec3 lightDirectionalImpl(Material material, UniLight light, float shadow)
{
	return lightingBrdf(
		material,
		-light.direction.xyz,
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * light.color.rgb * shadow;
}

vec3 lightPointImpl(Material material, UniLight light, float shadow)
{
	float att = attenuation(light.attenuation.xyz, length(light.position.xyz - varPosition));
	return lightingBrdf(
		material,
		normalize(light.position.xyz - varPosition),
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * light.color.rgb * att * shadow;
}

vec3 lightSpotImpl(Material material, UniLight light, float shadow)
{
	float att = attenuation(light.attenuation.xyz, length(light.position.xyz - varPosition));
	vec3 lightSourceToFragmentDirection = normalize(light.position.xyz - varPosition);
	float d = max(dot(-light.direction.xyz, lightSourceToFragmentDirection), 0);
	float a = light.parameters[0]; // angle
	float e = light.parameters[1]; // exponent
	if (d < a)
		return vec3(0);
	d = pow(d, e);
	return lightingBrdf(
		material,
		lightSourceToFragmentDirection,
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * light.color.rgb * d * att * shadow;
}

vec4 shadowSamplingPosition4(UniLight light)
{
	float normalOffsetScale = light.parameters[2];
	vec3 p3 = varPosition + normal * normalOffsetScale;
	return uniShadowMatrix * vec4(p3, 1);
}

vec3 lightDirectionalShadow(Material material, UniLight light)
{
	vec3 shadowPos = vec3(shadowSamplingPosition4(light));
	return lightDirectionalImpl(material, light, sampleShadowMap2d(texShadow2d, shadowPos));
}

vec3 lightPointShadow(Material material, UniLight light)
{
	vec3 shadowPos = vec3(shadowSamplingPosition4(light));
	return lightPointImpl(material, light, sampleShadowMapCube(texShadowCube, shadowPos));
}

vec3 lightSpotShadow(Material material, UniLight light)
{
	vec4 shadowPos4 = shadowSamplingPosition4(light);
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;
	return lightSpotImpl(material, light, sampleShadowMap2d(texShadow2d, shadowPos));
}

vec3 lightType(Material material, UniLight light)
{
	switch (uint(light.parameters[3]))
	{
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL: return lightDirectionalImpl(material, light, 1);
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW: return lightDirectionalShadow(material, light);
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINT: return lightPointImpl(material, light, 1);
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW: return lightPointShadow(material, light);
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOT: return lightSpotImpl(material, light, 1);
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW: return lightSpotShadow(material, light);
	default: return vec3(191, 85, 236) / 255;
	}
}

vec3 lightAmbient(Material material, float shadow)
{
	vec3 d = lightingBrdf(
		material,
		-uniViewport.eyeDir.xyz,
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * uniViewport.ambientDirectionalLight.rgb * shadow;
	vec3 a = material.albedo * uniViewport.ambientLight.rgb * shadow;
	vec3 e = material.albedo * material.emissive;
	return d + a + e;
}

vec4 lighting(Material material)
{
	vec4 res;
	if (dot(normal, normal) < 0.5)
	{
		// lighting is disabled
		res.rgb = lightAmbient(material, 0); // emissive only
	}
	else
	{
		res.rgb = lightAmbient(material, 1); // todo ambient occlusion
		for (int i = 0; i < uniLightsCount; i++)
			res.rgb += lightType(material, uniLights[i]);
	}
	res.a = material.opacity;
	return res;
}

vec4 restoreNormalMapZ(vec4 n)
{
	n.z = sqrt(1 - clamp(dot(n.xy, n.xy), 0, 1));
	return n;
}

vec4 matMapImpl(int index)
{
	switch (uniRoutines[index])
	{
		case 0: return vec4(0, 0, 0, 1);
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D: return texture(texMaterialAlbedo2d, varUv.xy);
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY: return sampleTextureAnimation(texMaterialAlbedoArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames);
		case CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE: return texture(texMaterialAlbedoCube, varUv);
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D: return texture(texMaterialSpecial2d, varUv.xy);
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY: return sampleTextureAnimation(texMaterialSpecialArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames);
		case CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE: return texture(texMaterialSpecialCube, varUv);
		case CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D: return restoreNormalMapZ(texture(texMaterialNormal2d, varUv.xy));
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY: return restoreNormalMapZ(sampleTextureAnimation(texMaterialNormalArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames));
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE: return restoreNormalMapZ(texture(texMaterialNormalCube, varUv));
		default: return vec4(-100);
	}
}

void updateNormal()
{
	normal = normalize(varNormal);

	if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] != 0)
	{
		vec4 normalMap = matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL);
		normal = mat3(normalize(varTangent), normalize(varBitangent), normal) * (normalMap.xyz * 2 - 1);
	}

	if (!gl_FrontFacing)
		normal *= -1;

	mat3x4 nm = uniMeshes[varInstanceId].normalMat;
	if (nm[2][3] > 0.5) // is lighting enabled
		normal = normalize(mat3(nm) * normal);
	else
		normal = vec3(0);
}

Material loadMaterial()
{
	Material material;

	vec4 specialMap = matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL);
	vec4 special4 = uniMaterial.specialBase + specialMap * uniMaterial.specialMult;
	material.roughness = special4.r;
	material.metalness = special4.g;
	material.emissive = special4.b;

	vec4 albedoMap = matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO);
	if (albedoMap.a > 1e-6)
		albedoMap.rgb /= albedoMap.a; // depremultiply albedo texture
	else
		albedoMap.a = 0;
	vec4 albedo4 = uniMaterial.albedoBase + albedoMap * uniMaterial.albedoMult;
	material.albedo = albedo4.rgb;
	material.opacity = albedo4.a;

	material.albedo = mix(uniMeshes[varInstanceId].color.rgb, material.albedo, special4.a);
	material.opacity *= uniMeshes[varInstanceId].color.a;
	
	if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_OPAQUE] == 0)
		material.albedo *= material.opacity; // premultiplied alpha
	else if (material.opacity < 0.5)
		discard;

	return material;
}
