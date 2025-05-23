
$define shader fragment

$include ../functions/common.glsl
$include ../functions/attenuation.glsl
$include ../functions/brdf.glsl
$include ../functions/makeTangentSpace.glsl
$include ../functions/restoreNormalMap.glsl
$include ../functions/sampleShadowMap.glsl
$include ../functions/sampleTextureAnimation.glsl

$include uniforms.glsl

in vec3 varPosition; // world space
in vec3 varNormal; // object space
in vec3 varUv;
flat in int varInstanceId;

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

vec3 egacLightBrdf(Material material, UniLight light)
{
	int lightType = int(light.params[0]);
	vec3 L;
	if (lightType == CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONAL || lightType == CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONALSHADOW)
		L = -light.direction.xyz;
	else
		L = normalize(light.position.xyz - varPosition);
	vec3 V = normalize(uniViewport.eyePos.xyz - varPosition);
	vec3 res = brdf(normal, L, V, material.albedo, material.roughness, material.metallic);
	if (lightType == CAGE_SHADER_OPTIONVALUE_LIGHTSPOT || lightType == CAGE_SHADER_OPTIONVALUE_LIGHTSPOTSHADOW)
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

float egacShadowedIntensity(UniShadowedLight uni)
{
	float normalOffsetScale = uni.shadowParams[2];
	vec3 p3 = varPosition + normal * normalOffsetScale;

	float viewDepth = -(uniProjection.viewMat * vec4(p3, 1)).z;
	int cascade = 0;
	for (int i = 0; i < CAGE_SHADER_MAX_SHADOWMAPSCASCADES; i++)
	{
		if (viewDepth < uni.cascadesDepths[i])
		{
			cascade = i;
			break;
		}
	}
	// return float(cascade) / float(CAGE_SHADER_MAX_SHADOWMAPSCASCADES); // debug

	vec4 shadowPos4 = uni.shadowMat[cascade] * vec4(p3, 1);
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;

	int shadowmapSamplerIndex = int(uni.shadowParams[0]);
	float intensity = 1;
	switch (int(uni.light.params[0]))
	{
	case CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONALSHADOW:
	case CAGE_SHADER_OPTIONVALUE_LIGHTSPOTSHADOW:
		intensity = sampleShadowMap2d(texShadows2d[shadowmapSamplerIndex], shadowPos, cascade);
		break;
	case CAGE_SHADER_OPTIONVALUE_LIGHTPOINTSHADOW:
		intensity = sampleShadowMapCube(texShadowsCube[shadowmapSamplerIndex], shadowPos);
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

	if (dot(normal, normal) > 0.5) // lighting enabled
	{
		float ssao = 1;
		if (getOption(CAGE_SHADER_OPTIONINDEX_AMBIENTOCCLUSION) > 0)
			ssao = egacLightAmbientOcclusion();

		// ambient
		res.rgb += material.albedo * uniViewport.ambientLight.rgb * ssao;

		// sky (compensate for lack of IBL)
		res.rgb += egacSkyContribution(material) * ssao;

		{ // direct unshadowed
			int lightsCount = getOption(CAGE_SHADER_OPTIONINDEX_LIGHTSCOUNT);
			for (int i = 0; i < lightsCount; i++)
			{
				UniLight light = uniLights[i];
				float intensity = egacLightInitialIntensity(light, ssao);
				if (intensity < CAGE_SHADER_MAX_LIGHTINTENSITYTHRESHOLD)
					continue;
				res.rgb += egacLightBrdf(material, light) * intensity;
			}
		}

		{ // direct shadowed
			int lightsCount = getOption(CAGE_SHADER_OPTIONINDEX_SHADOWEDLIGHTSCOUNT);
			for (int i = 0; i < lightsCount; i++)
			{
				UniShadowedLight uni = uniShadowedLights[i];
				float intensity = egacLightInitialIntensity(uni.light, ssao);
				if (intensity < CAGE_SHADER_MAX_LIGHTINTENSITYTHRESHOLD)
					continue;
				intensity *= egacShadowedIntensity(uni);
				if (intensity < CAGE_SHADER_MAX_LIGHTINTENSITYTHRESHOLD)
					continue;
				res.rgb += egacLightBrdf(material, uni.light) * intensity;
			}
		}
	}

	if (material.fade < 0.5)
		res.a = material.opacity;
	else
		res *= material.opacity;

	return res;
}

vec4 egacMatMapImpl(int index)
{
	switch (getOption(index))
	{
		case 0: return vec4(0, 0, 0, 1);
		case CAGE_SHADER_OPTIONVALUE_MAPALBEDO2D: return texture(texMaterialAlbedo2d, varUv.xy);
		case CAGE_SHADER_OPTIONVALUE_MAPALBEDOARRAY: return sampleTextureAnimation(texMaterialAlbedoArray, varUv.xy, uniMeshes[varInstanceId].animation, uniMaterial.animation);
		case CAGE_SHADER_OPTIONVALUE_MAPALBEDOCUBE: return texture(texMaterialAlbedoCube, varUv);
		case CAGE_SHADER_OPTIONVALUE_MAPSPECIAL2D: return texture(texMaterialSpecial2d, varUv.xy);
		case CAGE_SHADER_OPTIONVALUE_MAPSPECIALARRAY: return sampleTextureAnimation(texMaterialSpecialArray, varUv.xy, uniMeshes[varInstanceId].animation, uniMaterial.animation);
		case CAGE_SHADER_OPTIONVALUE_MAPSPECIALCUBE: return texture(texMaterialSpecialCube, varUv);
		case CAGE_SHADER_OPTIONVALUE_MAPNORMAL2D: return texture(texMaterialNormal2d, varUv.xy);
		case CAGE_SHADER_OPTIONVALUE_MAPNORMALARRAY: return sampleTextureAnimation(texMaterialNormalArray, varUv.xy, uniMeshes[varInstanceId].animation, uniMaterial.animation);
		case CAGE_SHADER_OPTIONVALUE_MAPNORMALCUBE: return texture(texMaterialNormalCube, varUv);
		default: return vec4(-100);
	}
}

Material loadMaterial()
{
	Material material;

	vec4 specialMap = egacMatMapImpl(CAGE_SHADER_OPTIONINDEX_MAPSPECIAL);
	vec4 special4 = uniMaterial.specialBase + specialMap * uniMaterial.specialMult;
	material.roughness = special4.r;
	material.metallic = special4.g;
	material.emissive = special4.b;

	vec4 albedoMap = egacMatMapImpl(CAGE_SHADER_OPTIONINDEX_MAPALBEDO);
	if (albedoMap.a > 1e-6)
		albedoMap.rgb /= albedoMap.a; // depremultiply albedo texture
	else
		albedoMap.a = 0;
	vec4 albedo4 = uniMaterial.albedoBase + albedoMap * uniMaterial.albedoMult;
	material.albedo = albedo4.rgb;
	material.opacity = albedo4.a;

	material.albedo = mix(uniMeshes[varInstanceId].color.rgb, material.albedo, special4.a);
	material.opacity *= uniMeshes[varInstanceId].color.a;
	material.fade = uniMaterial.lighting[1];

	return material;
}

// converts normal from object to world space
// additionally applies normal map
void updateNormal()
{
	normal = normalize(varNormal);

	if (getOption(CAGE_SHADER_OPTIONINDEX_MAPNORMAL) != 0)
	{
		vec3 normalMap = restoreNormalMap(egacMatMapImpl(CAGE_SHADER_OPTIONINDEX_MAPNORMAL));
		mat3 tbn = makeTangentSpace(normal, varPosition - uniViewport.eyePos.xyz, varUv.xy);
		normal = tbn * normalMap;
	}

	if (!gl_FrontFacing)
		normal *= -1;

	if (uniMaterial.lighting[0] > 0.5) // is lighting enabled
	{
		mat3 nm = transpose(mat3(uniMeshes[varInstanceId].modelMat));
		normal = normalize(nm * normal);
	}
	else
		normal = vec3(0);
}
