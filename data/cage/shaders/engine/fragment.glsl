
$define shader fragment

$include ../functions/attenuation.glsl
$include ../functions/brdf.glsl
$include ../functions/makeTangentSpace.glsl
$include ../functions/restoreNormalMap.glsl
$include ../functions/sampleShadowMap.glsl
$include ../functions/sampleTextureAnimation.glsl

layout(location = 0) out vec4 outColor;

vec3 normal;

struct Material
{
	vec3 albedo; // linear, not-premultiplied
	float opacity;
	float roughness;
	float metallic;
	float emissive;
	float fade;
};

Material defaultMaterial()
{
	Material m;
	m.albedo = vec3(1);
	m.opacity = 1;
	m.roughness = 1;
	m.metallic = 0;
	m.emissive = 0;
	m.fade = 0;
	return m;
}

vec3 egacLightBrdf(Material material, UniLight light)
{
	int lightType = int(light.params[0]);
	vec3 L;
	if (lightType == 11 || lightType == 12)
		L = -light.direction.xyz;
	else
		L = normalize(light.position.xyz - varPosition);
	vec3 V = normalize(uniViewport.eyePos.xyz - varPosition);
	vec3 res = brdf(normal, L, V, material.albedo, material.roughness, material.metallic);
	if (lightType == 13 || lightType == 14)
	{
		float d = max(dot(-light.direction.xyz, L), 0);
		if (d < light.params[2])
			d = 0;
		else
			d = pow(d, light.params[3]);
		res *= d;
	}
	return res * light.color.rgb;
}

float egacSampleShadowmap2d(int index, vec3 shadowPos, int cascade)
{
	switch (index)
	{
		case 0: return sampleShadowMap2d(texShadows2d_0, shadowPos, cascade);
		case 1: return sampleShadowMap2d(texShadows2d_1, shadowPos, cascade);
		case 2: return sampleShadowMap2d(texShadows2d_2, shadowPos, cascade);
		case 3: return sampleShadowMap2d(texShadows2d_3, shadowPos, cascade);
		case 4: return sampleShadowMap2d(texShadows2d_4, shadowPos, cascade);
		case 5: return sampleShadowMap2d(texShadows2d_5, shadowPos, cascade);
		case 6: return sampleShadowMap2d(texShadows2d_6, shadowPos, cascade);
		case 7: return sampleShadowMap2d(texShadows2d_7, shadowPos, cascade);
	}
	return 0;
}

float egacSampleShadowmapCube(int index, vec3 shadowPos)
{
	switch (index)
	{
		case 0: return sampleShadowMapCube(texShadowsCube_0, shadowPos);
		case 1: return sampleShadowMapCube(texShadowsCube_1, shadowPos);
		case 2: return sampleShadowMapCube(texShadowsCube_2, shadowPos);
		case 3: return sampleShadowMapCube(texShadowsCube_3, shadowPos);
		case 4: return sampleShadowMapCube(texShadowsCube_4, shadowPos);
		case 5: return sampleShadowMapCube(texShadowsCube_5, shadowPos);
		case 6: return sampleShadowMapCube(texShadowsCube_6, shadowPos);
		case 7: return sampleShadowMapCube(texShadowsCube_7, shadowPos);
	}
	return 0;
}

float egacShadowedIntensity(UniShadowedLight uni)
{
	float normalOffsetScale = uni.shadowParams[2];
	vec3 p3 = varPosition + normal * normalOffsetScale;

	float viewDepth = -(uniProjection.viewMat * vec4(p3, 1)).z;
	int cascade = 0;
	for (int i = 0; i < 4; i++)
	{
		if (viewDepth < uni.cascadesDepths[i])
		{
			cascade = i;
			break;
		}
	}
	// return float(cascade) / float(3); // debug

	vec4 shadowPos4 = uni.shadowMat[cascade] * vec4(p3, 1);
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;

	int shadowmapSamplerIndex = int(uni.shadowParams[0]);
	float intensity = 1;
	switch (int(uni.light.params[0]))
	{
	case 12:
	case 14:
		intensity = egacSampleShadowmap2d(shadowmapSamplerIndex, shadowPos, cascade);
		break;
	case 16:
		intensity = egacSampleShadowmapCube(shadowmapSamplerIndex, shadowPos);
		break;
	}
	return mix(1, intensity, uni.shadowParams[3]);
}

float egacLightInitialIntensity(UniLight light, float ssao)
{
	float intensity = light.color[3];
	intensity *= attenuation(light.attenuation, length(light.position.xyz - varPosition));
	intensity *= mix(1, ssao, light.params[1]);
	return intensity;
}

float egacLightAmbientOcclusion()
{
	return textureLod(texSsao, vec2(gl_FragCoord.xy) / uniViewport.viewport.zw, 0).x;
}

// this is fake contribution to compensate for missing reflections from light probes
// it instead adds sky color
vec3 egacSkyContribution(Material material)
{
	vec3 F0 = mix(vec3(0.04), material.albedo, material.metallic);
	vec3 V = normalize(uniViewport.eyePos.xyz - varPosition);
	float NoV = saturate(dot(normal, V));
	vec3 fresnel = egacFresnelSchlick(F0, NoV * NoV);
	float rf = pow(1 - material.roughness, 3) * 0.05; // tweakable parameters
	return uniViewport.skyLight.rgb * fresnel * rf;
}

vec4 lighting(Material material)
{
	if (material.fade < 0.5)
		material.albedo *= material.opacity;

	vec4 res = vec4(0, 0, 0, 1);

	// emissive
	res.rgb += material.albedo * material.emissive;

#ifdef Normals
	if (dot(normal, normal) > 0.5) // lighting enabled
	{
		float ssao = 1;
		if (uniOptsLights[2] > 0.5)
			ssao = egacLightAmbientOcclusion();

		// ambient
		res.rgb += material.albedo * uniViewport.ambientLight.rgb * ssao;

		// sky (compensate for lack of IBL)
		res.rgb += egacSkyContribution(material) * ssao;

		{ // direct unshadowed
			int lightsCount = uniOptsLights[0];
			for (int i = 0; i < lightsCount; i++)
			{
				UniLight light = uniLights[i];
				float intensity = egacLightInitialIntensity(light, ssao);
				if (intensity < 0.05)
					continue;
				res.rgb += egacLightBrdf(material, light) * intensity;
			}
		}

		{ // direct shadowed
			int lightsCount = uniOptsLights[1];
			for (int i = 0; i < lightsCount; i++)
			{
				UniShadowedLight uni = uniShadowedLights[i];
				float intensity = egacLightInitialIntensity(uni.light, ssao);
				if (intensity < 0.05)
					continue;
				intensity *= egacShadowedIntensity(uni);
				if (intensity < 0.05)
					continue;
				res.rgb += egacLightBrdf(material, uni.light) * intensity;
			}
		}
	}
#endif // Normals

	if (material.fade < 0.5)
		res.a = material.opacity;
	else
		res *= material.opacity;

	return res;
}

vec4 loadAlbedo()
{
#ifdef MaterialTexRegular
	return texture(texMaterialAlbedo, varUv.xy);
#endif // MaterialTexRegular
#ifdef MaterialTexArray
	return sampleTextureAnimation(texMaterialAlbedo, varUv.xy, uniMeshes[varInstanceId].animation, uniMaterial.animation);
#endif // MaterialTexArray
#ifdef MaterialTexCube
	return texture(texMaterialAlbedo, varUv);
#endif // MaterialTexCube
	return vec4(0, 0, 0, 1);
}

vec4 loadSpecial()
{
#ifdef MaterialTexRegular
	return texture(texMaterialSpecial, varUv.xy);
#endif // MaterialTexRegular
#ifdef MaterialTexArray
	return sampleTextureAnimation(texMaterialSpecial, varUv.xy, uniMeshes[varInstanceId].animation, uniMaterial.animation);
#endif // MaterialTexArray
#ifdef MaterialTexCube
	return texture(texMaterialSpecial, varUv);
#endif // MaterialTexCube
	return vec4(0, 0, 0, 1);
}

vec4 loadNormalMap()
{
#ifdef MaterialTexRegular
	return texture(texMaterialNormal, varUv.xy);
#endif // MaterialTexRegular
#ifdef MaterialTexArray
	return sampleTextureAnimation(texMaterialNormal, varUv.xy, uniMeshes[varInstanceId].animation, uniMaterial.animation);
#endif // MaterialTexArray
#ifdef MaterialTexCube
	return texture(texMaterialNormal, varUv);
#endif // MaterialTexCube
	return vec4(0, 0, 0, 1);
}

Material loadMaterial()
{
	Material material;

	vec4 albedoMap = loadAlbedo();
	if (albedoMap.a > 1e-6)
		albedoMap.rgb /= albedoMap.a; // depremultiply albedo texture
	else
		albedoMap.a = 0;
	vec4 albedo4 = uniMaterial.albedoBase + albedoMap * uniMaterial.albedoMult;
	material.albedo = albedo4.rgb;
	material.opacity = albedo4.a;

	vec4 specialMap = loadSpecial();
	vec4 special4 = uniMaterial.specialBase + specialMap * uniMaterial.specialMult;
	material.roughness = special4.r;
	material.metallic = special4.g;
	material.emissive = special4.b;

	material.albedo = mix(uniMeshes[varInstanceId].color.rgb, material.albedo, special4.a);
	material.opacity *= uniMeshes[varInstanceId].color.a;
	material.fade = uniMaterial.lighting[1];

	return material;
}

// converts normal from object to world space
// additionally applies normal map
void updateNormal()
{
#ifdef Normals

	normal = normalize(varNormal);

#if defined(MaterialTexRegular) || defined(MaterialTexArray) || defined(MaterialTexCube)
	if (uniOptsLights[3] > 0.5)
	{
		vec3 normalMap = restoreNormalMap(loadNormalMap());
		mat3 tbn = makeTangentSpace(normal, varPosition - uniViewport.eyePos.xyz, varUv.xy);
		normal = tbn * normalMap;
	}
#endif

	if (!gl_FrontFacing)
		normal *= -1;

	if (uniMaterial.lighting[0] > 0.5) // is lighting enabled
	{
		mat3 nm = transpose(mat3(uniMeshes[varInstanceId].modelMat));
		normal = normalize(nm * normal);
	}
	else
		normal = vec3(0);

#else

	normal = vec3(0);

#endif // Normals
}
