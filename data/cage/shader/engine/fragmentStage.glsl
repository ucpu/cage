
$include includes.glsl
$include ../functions/poissonDisk4.glsl

layout(binding = CAGE_SHADER_TEXTURE_SHADOW) uniform sampler2D texShadow2d;
layout(binding = CAGE_SHADER_TEXTURE_SHADOW_CUBE) uniform samplerCube texShadowCube;

float sampleShadowMap2dFast(vec3 shadowPos)
{
	return float(texture(texShadow2d, shadowPos.xy).x > shadowPos.z);
}

float randomAngle(float freq, vec3 pos)
{
	float dt = dot(floor(pos * freq), vec3(53.1215, 21.1352, 9.1322));
	return fract(sin(dt) * 2105.2354) * 6.283285;
}

float sampleShadowMap2d(vec3 shadowPos)
{
	vec2 radius = 0.8 / textureSize(texShadow2d, 0).xy;
	float visibility = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float ra = randomAngle(12000.0, shadowPos);
		mat2 rot = mat2(cos(ra), sin(ra), -sin(ra), cos(ra));
		vec2 samp = rot * poissonDisk4[i] * radius;
		visibility += float(texture(texShadow2d, shadowPos.xy + samp).x > shadowPos.z);
	}
	visibility /= 4.0;
	return visibility;
}

float sampleShadowMapCube(vec3 shadowPos)
{
	return float(texture(texShadowCube, normalize(shadowPos)).x > length(shadowPos));
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float distributionGGX(float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	float denom = NoH2 * (a2 - 1.0) + 1.0;
	denom = 3.14159265359 * denom * denom;
	return a2 / denom;
}
float geometrySchlickGGX(float NoV)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	float denom = NoV * (1.0 - k) + k;
	return NoV / denom;
}
float geometrySmith(float NoL, float NoV)
{
	float ggx1 = geometrySchlickGGX(NoL);
	float ggx2 = geometrySchlickGGX(NoV);
	return ggx1 * ggx2;
}

vec3 lightingBrdfPbr(vec3 light, vec3 L, vec3 V)
{
	vec3 N = normal;
	vec3 H = normalize(L + V);
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);
	float NoH = max(dot(N, H), 0.0); 
	float VoH = max(dot(V, H), 0.0);

	// https://learnopengl.com/PBR/Lighting
	// https://github.com/pumexx/pumex/blob/86fda7fa351d00bd5918ad90899ce2d6bb8b1dfe/examples/pumexdeferred/shaders/deferred_composite.frag
	vec3 F0 = mix(vec3(0.04), albedo, metalness);
	vec3 F = fresnelSchlick(VoH, F0); // VoH or NoV
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;

	float NDF = distributionGGX(NoH);
	float G = geometrySmith(NoL, NoV);

	vec3 nominator = NDF * G * F;
	float denominator = 4.0 * NoV * NoL + 0.001;
	return min(kD * albedo / 3.14159265359 + nominator / denominator, 1.0) * light * NoL;
}

vec3 lightingBrdf(vec3 light, vec3 L, vec3 V)
{
	return lightingBrdfPbr(light, L, V);
}

layout(std140, binding = CAGE_SHADER_UNIBLOCK_LIGHT) uniform Light
{
	mat4 shadowMat;
	vec4 color; // + spotAngle
	vec4 position;
	vec4 direction; // + normalOffsetScale
	vec4 attenuation; // + spotExponent
} uniLight;

float attenuation(float dist)
{
	vec3 att = uniLight.attenuation.xyz;
	return 1.0 / (att.x + dist * (att.y + dist * att.z));
}

vec3 lightDirectionalImpl(float shadow)
{
	return lightingBrdf(
		uniLight.color.rgb * shadow,
		-uniLight.direction.xyz,
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightPointImpl(float shadow)
{
	vec3 light = uniLight.color.rgb * attenuation(length(uniLight.position.xyz - position));
	return lightingBrdf(
		light * shadow,
		normalize(uniLight.position.xyz - position),
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightSpotImpl(float shadow)
{
	vec3 light = uniLight.color.rgb * attenuation(length(uniLight.position.xyz - position));
	vec3 lightSourceToFragmentDirection = normalize(uniLight.position.xyz - position);
	float d = max(dot(-uniLight.direction.xyz, lightSourceToFragmentDirection), 0.0);
	float a = uniLight.color[3]; // angle
	float e = uniLight.attenuation[3]; // exponent
	if (d < a)
		return vec3(0);
	d = pow(d, e);
	return lightingBrdf(
		light * d * shadow,
		lightSourceToFragmentDirection,
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightAmbientImpl(float intensity)
{
	vec3 d = lightingBrdf(
		uniAmbientDirectionalLight.rgb * intensity,
		-uniEyeDir.xyz,
		normalize(uniEyePos.xyz - position)
	);
	vec3 a = albedo * uniAmbientLight.rgb * intensity;
	return a + d;
}

vec3 lightEmissiveImpl()
{
	return albedo * emissive;
}

vec4 shadowSamplingPosition4()
{
	float normalOffsetScale = uniLight.direction[3];
	vec3 p3 = position + normal * normalOffsetScale;
	return uniLight.shadowMat * vec4(p3, 1.0);
}

vec3 lightDirectional()
{
	return lightDirectionalImpl(1);
}

vec3 lightDirectionalShadow()
{
	vec3 shadowPos = vec3(shadowSamplingPosition4());
	return lightDirectionalImpl(sampleShadowMap2d(shadowPos));
}

vec3 lightPoint()
{
	return lightPointImpl(1);
}

vec3 lightPointShadow()
{
	vec3 shadowPos = vec3(shadowSamplingPosition4());
	return lightPointImpl(sampleShadowMapCube(shadowPos));
}

vec3 lightSpot()
{
	return lightSpotImpl(1);
}

vec3 lightSpotShadow()
{
	vec4 shadowPos4 = shadowSamplingPosition4();
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;
	return lightSpotImpl(sampleShadowMap2d(shadowPos));
}

vec3 lightForwardBase()
{
	return lightAmbientImpl(1);
}

vec3 lightType()
{
	if (dot(normal, normal) < 0.5)
		return vec3(0.0); // lighting is disabled
	switch (uniRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE])
	{
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL: return lightDirectional();
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW: return lightDirectionalShadow();
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINT: return lightPoint();
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW: return lightPointShadow();
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOT: return lightSpot();
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW: return lightSpotShadow();
	case CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE: return lightForwardBase();
	default: return vec3(191.0, 85.0, 236.0) / 255.0;
	}
}

vec4 sampleTextureAnimation(sampler2DArray tex, vec2 uv, vec4 frames)
{
	vec4 a = texture(tex, vec3(uv, frames.x));
	vec4 b = texture(tex, vec3(uv, frames.y));
	return mix(a, b, frames.z);
}

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
	return sampleTextureAnimation(texMaterialAlbedoArray, uv.xy, uniInstances[varInstanceId].aniTexFrames);
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
	return sampleTextureAnimation(texMaterialSpecialArray, uv.xy, uniInstances[varInstanceId].aniTexFrames);
}

vec4 materialMapSpecialCube()
{
	return texture(texMaterialSpecialCube, uv);
}

vec4 restoreNormalMapZ(vec4 n)
{
	n.z = sqrt(1 - clamp(dot(n.xy, n.xy), 0, 1));
	return n;
}

vec4 materialMapNormal2d()
{
	return restoreNormalMapZ(texture(texMaterialNormal2d, uv.xy));
}

vec4 materialMapNormalArray()
{
	return restoreNormalMapZ(sampleTextureAnimation(texMaterialNormalArray, uv.xy, uniInstances[varInstanceId].aniTexFrames));
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

	albedo = mix(uniInstances[varInstanceId].color.rgb, albedo, special4.a);
	opacity *= uniInstances[varInstanceId].color.a;
	albedo *= opacity; // premultiplied alpha
}

void normalLoad()
{
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
	mat3x4 nm = uniInstances[varInstanceId].normalMat;
	if (nm[2][3] > 0.5) // is lighting enabled
		normal = normalize(mat3(nm) * normal);
	else
		normal = vec3(0.0);
}
