
$define shader fragment

$include ../functions/brdf.glsl
$include ../functions/attenuation.glsl
$include ../functions/sampleShadowMap.glsl
$include ../functions/sampleTextureAnimation.glsl

$include uniforms.glsl

in vec3 varPosition; // world space
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varUv;
flat in int varInstanceId;

layout(location = 0) out vec4 outColor;

vec3 normal;

const float confLightThreshold = 0.05;

struct Material
{
	vec3 albedo; // linear, not-premultiplied
	float opacity;
	float roughness;
	float metalness;
	float emissive;
	float fade;
};

vec3 lightingBrdf(Material material, vec3 L, vec3 V)
{
	return brdf(normal, L, V, material.albedo, material.roughness, material.metalness);
}

vec3 lightDirectionalImpl(Material material, UniLight light, float intensity)
{
	return lightingBrdf(
		material,
		-light.direction.xyz,
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * light.color.rgb * intensity;
}

vec3 lightPointImpl(Material material, UniLight light, float intensity)
{
	return lightingBrdf(
		material,
		normalize(light.position.xyz - varPosition),
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * light.color.rgb * intensity;
}

vec3 lightSpotImpl(Material material, UniLight light, float intensity)
{
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
	) * light.color.rgb * d * intensity;
}

vec4 shadowSamplingPosition4(UniLight light, uint shadowIndex)
{
	float normalOffsetScale = light.parameters[2];
	vec3 p3 = varPosition + normal * normalOffsetScale;
	return uniShadowsMatrices[shadowIndex] * vec4(p3, 1);
}

sampler2D pickShadowSampler2d(uint shadowIndex)
{
	if ((shadowIndex % 2) == 0)
		return sampler2D(texShadows2d[shadowIndex / 2].xy);
	else
		return sampler2D(texShadows2d[shadowIndex / 2].zw);
}

samplerCube pickShadowSamplerCube(uint shadowIndex)
{
	if ((shadowIndex % 2) == 0)
		return samplerCube(texShadowsCube[shadowIndex / 2].xy);
	else
		return samplerCube(texShadowsCube[shadowIndex / 2].zw);
}

vec3 lightDirectionalShadow(Material material, UniLight light, uint shadowIndex)
{
	vec3 shadowPos = vec3(shadowSamplingPosition4(light, shadowIndex));
	float shadow = sampleShadowMap2d(pickShadowSampler2d(shadowIndex), shadowPos);
	return lightDirectionalImpl(material, light, shadow);
}

vec3 lightPointShadow(Material material, UniLight light, float att, uint shadowIndex)
{
	vec3 shadowPos = vec3(shadowSamplingPosition4(light, shadowIndex));
	float shadow = sampleShadowMapCube(pickShadowSamplerCube(shadowIndex), shadowPos);
	return lightPointImpl(material, light, shadow * att);
}

vec3 lightSpotShadow(Material material, UniLight light, float att, uint shadowIndex)
{
	vec4 shadowPos4 = shadowSamplingPosition4(light, shadowIndex);
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;
	float shadow = sampleShadowMap2d(pickShadowSampler2d(shadowIndex), shadowPos);
	return lightSpotImpl(material, light, shadow * att);
}

vec3 lightType(Material material, UniLight light, uint shadowIndex)
{
	float att = attenuation(light.attenuation.xyz, length(light.position.xyz - varPosition));
	if (att < confLightThreshold)
		return vec3(0);
	switch (uint(light.parameters[3]))
	{
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL: return lightDirectionalImpl(material, light, 1);
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW: return lightDirectionalShadow(material, light, shadowIndex);
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINT: return lightPointImpl(material, light, att);
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW: return lightPointShadow(material, light, att, shadowIndex);
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOT: return lightSpotImpl(material, light, att);
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW: return lightSpotShadow(material, light, att, shadowIndex);
	default: return vec3(191, 85, 236) / 255;
	}
}

vec3 lightAmbient(Material material)
{
	vec3 d = lightingBrdf(
		material,
		-uniViewport.eyeDir.xyz,
		normalize(uniViewport.eyePos.xyz - varPosition)
	) * uniViewport.ambientDirectionalLight.rgb;
	vec3 a = material.albedo * uniViewport.ambientLight.rgb;
	return d + a;
}

float lightAmbientOcclusion()
{
	return textureLod(texSsao, vec2(gl_FragCoord.xy) / uniViewport.viewport.zw, 0).x;
}

vec4 lighting(Material material)
{
	if (material.fade < 0.5)
		material.albedo *= material.opacity;

	vec4 res = vec4(0, 0, 0, 1);

	// emissive
	res.rgb += material.albedo * material.emissive;

	if (dot(normal, normal) > 0.5)
	{
		{ // ambient
			float ssao = 1;
			if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_AMBIENTOCCLUSION] > 0)
				ssao = lightAmbientOcclusion();
			res.rgb += lightAmbient(material) * ssao;
		}

		{ // direct
			for (int i = 0; i < uniLightsCount; i++)
				res.rgb += lightType(material, uniLights[i], i);
		}
	}

	if (material.fade < 0.5)
		res.a = material.opacity;
	else
		res *= material.opacity;

	return res;
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
		case CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D: return texture(texMaterialNormal2d, varUv.xy);
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY: return sampleTextureAnimation(texMaterialNormalArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames);
		case CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE: return texture(texMaterialNormalCube, varUv);
		default: return vec4(-100);
	}
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
	material.fade = uniMeshes[varInstanceId].normalMat[1][3];

	return material;
}

vec3 restoreNormalMap(vec4 n)
{
	n.xy = n.xy * 2 - 1;
	n.z = sqrt(1 - clamp(dot(n.xy, n.xy), 0, 1));
	return n.xyz;
}

// converts normal from object to world space
// additionally applies normal map
void updateNormal()
{
	normal = normalize(varNormal);

	if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] != 0)
	{
		vec3 normalMap = restoreNormalMap(matMapImpl(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL));
		vec3 tangent = normalize(varTangent);
		vec3 bitangent = cross(normal, tangent);
		normal = mat3(tangent, bitangent, normal) * normalMap;
	}

	if (!gl_FrontFacing)
		normal *= -1;

	mat3x4 nm = uniMeshes[varInstanceId].normalMat;
	if (nm[2][3] > 0.5) // is lighting enabled
		normal = normalize(mat3(nm) * normal);
	else
		normal = vec3(0);
}
