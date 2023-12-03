
$define shader fragment

$include ../functions/brdf.glsl
$include ../functions/attenuation.glsl
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
	float metalness;
	float emissive;
	float fade;
};

vec3 lightBrdf(Material material, UniLight light)
{
	vec3 L;
	if (light.iparams[0] == CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONAL || light.iparams[0] == CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONALSHADOW)
		L = -light.direction.xyz;
	else
		L = normalize(light.position.xyz - varPosition);
	vec3 V = normalize(uniViewport.eyePos.xyz - varPosition);
	vec3 res = brdf(normal, L, V, material.albedo, material.roughness, material.metalness);
	if (light.iparams[0] == CAGE_SHADER_OPTIONVALUE_LIGHTSPOT || light.iparams[0] == CAGE_SHADER_OPTIONVALUE_LIGHTSPOTSHADOW)
	{
		float d = max(dot(-light.direction.xyz, L), 0);
		if (d < light.fparams[0])
			d = 0;
		else
			d = pow(d, light.fparams[1]);
		res *= d;
	}
	return res * light.color.rgb;
}

float shadowedIntensity(UniShadowedLight uni)
{
	float normalOffsetScale = uni.light.fparams[2];
	vec3 p3 = varPosition + normal * normalOffsetScale;
	vec4 shadowPos4 = uni.shadowMat * vec4(p3, 1);
	switch (uni.light.iparams[0])
	{
	case CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONALSHADOW:
		return sampleShadowMap2d(texShadows2d[uni.light.iparams[1]], vec3(shadowPos4));
	case CAGE_SHADER_OPTIONVALUE_LIGHTPOINTSHADOW:
		return sampleShadowMapCube(texShadowsCube[uni.light.iparams[1]], vec3(shadowPos4));
	case CAGE_SHADER_OPTIONVALUE_LIGHTSPOTSHADOW:
		return sampleShadowMap2d(texShadows2d[uni.light.iparams[1]], shadowPos4.xyz / shadowPos4.w);
	default:
		return 1;
	}
}

float lightInitialIntensity(UniLight light, float ssao)
{
	float intensity = light.color[3];
	intensity *= attenuation(light.attenuation, length(light.position.xyz - varPosition));
	intensity *= mix(1.0, ssao, light.fparams[3]);
	return intensity;
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

	if (dot(normal, normal) > 0.5) // lighting enabled
	{
		float ssao = 1;
		if (getOption(CAGE_SHADER_OPTIONINDEX_AMBIENTOCCLUSION) > 0)
			ssao = lightAmbientOcclusion();

		// ambient
		res.rgb += material.albedo * uniViewport.ambientLight.rgb * ssao;

		{ // direct unshadowed
			int lightsCount = getOption(CAGE_SHADER_OPTIONINDEX_LIGHTSCOUNT);
			for (int i = 0; i < lightsCount; i++)
			{
				UniLight light = uniLights[i];
				float intensity = lightInitialIntensity(light, ssao);
				if (intensity < CAGE_SHADER_MAX_LIGHTINTENSITYTHRESHOLD)
					continue;
				res.rgb += lightBrdf(material, light) * intensity;
			}
		}

		{ // direct shadowed
			int lightsCount = getOption(CAGE_SHADER_OPTIONINDEX_SHADOWEDLIGHTSCOUNT);
			for (int i = 0; i < lightsCount; i++)
			{
				UniShadowedLight uni = uniShadowedLights[i];
				float intensity = lightInitialIntensity(uni.light, ssao);
				if (intensity < CAGE_SHADER_MAX_LIGHTINTENSITYTHRESHOLD)
					continue;
				intensity *= shadowedIntensity(uni);
				res.rgb += lightBrdf(material, uni.light) * intensity;
			}
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
	switch (getOption(index))
	{
		case 0: return vec4(0, 0, 0, 1);
		case CAGE_SHADER_OPTIONVALUE_MAPALBEDO2D: return texture(texMaterialAlbedo2d, varUv.xy);
		case CAGE_SHADER_OPTIONVALUE_MAPALBEDOARRAY: return sampleTextureAnimation(texMaterialAlbedoArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames);
		case CAGE_SHADER_OPTIONVALUE_MAPALBEDOCUBE: return texture(texMaterialAlbedoCube, varUv);
		case CAGE_SHADER_OPTIONVALUE_MAPSPECIAL2D: return texture(texMaterialSpecial2d, varUv.xy);
		case CAGE_SHADER_OPTIONVALUE_MAPSPECIALARRAY: return sampleTextureAnimation(texMaterialSpecialArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames);
		case CAGE_SHADER_OPTIONVALUE_MAPSPECIALCUBE: return texture(texMaterialSpecialCube, varUv);
		case CAGE_SHADER_OPTIONVALUE_MAPNORMAL2D: return texture(texMaterialNormal2d, varUv.xy);
		case CAGE_SHADER_OPTIONVALUE_MAPNORMALARRAY: return sampleTextureAnimation(texMaterialNormalArray, varUv.xy, uniMeshes[varInstanceId].aniTexFrames);
		case CAGE_SHADER_OPTIONVALUE_MAPNORMALCUBE: return texture(texMaterialNormalCube, varUv);
		default: return vec4(-100);
	}
}

Material loadMaterial()
{
	Material material;

	vec4 specialMap = matMapImpl(CAGE_SHADER_OPTIONINDEX_MAPSPECIAL);
	vec4 special4 = uniMaterial.specialBase + specialMap * uniMaterial.specialMult;
	material.roughness = special4.r;
	material.metalness = special4.g;
	material.emissive = special4.b;

	vec4 albedoMap = matMapImpl(CAGE_SHADER_OPTIONINDEX_MAPALBEDO);
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

mat3 makeTangentSpace(vec3 N, vec3 p, vec2 uv)
{
	// http://www.thetenthplanet.de/archives/1180
	uv.y = 1 - uv.y;
	vec3 dp1 = dFdx(p);
	vec3 dp2 = dFdy(p);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);
	vec3 dp2perp = cross(dp2, N);
	vec3 dp1perp = cross(N, dp1);
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
	float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
	return mat3(T * invmax, B * invmax, N);
}

// converts normal from object to world space
// additionally applies normal map
void updateNormal()
{
	normal = normalize(varNormal);

	if (getOption(CAGE_SHADER_OPTIONINDEX_MAPNORMAL) != 0)
	{
		vec3 normalMap = restoreNormalMap(matMapImpl(CAGE_SHADER_OPTIONINDEX_MAPNORMAL));
		mat3 tbn = makeTangentSpace(normal, varPosition - uniViewport.eyePos.xyz, varUv.xy);
		normal = tbn * normalMap;
	}

	if (!gl_FrontFacing)
		normal *= -1;

	mat3x4 nm = uniMeshes[varInstanceId].normalMat;
	if (nm[2][3] > 0.5) // is lighting enabled
		normal = normalize(mat3(nm) * normal);
	else
		normal = vec3(0);
}
