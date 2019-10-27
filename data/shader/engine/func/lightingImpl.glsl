
$include lightingBrdf.glsl

struct lightStruct
{
	mat4 shadowMat;
	mat4 mvpMat;
	vec4 color; // + angle
	vec4 position;
	vec4 direction;
	vec4 attenuation; // + exponent
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_LIGHTS) uniform Lights
{
	lightStruct uniLights[CAGE_SHADER_MAX_INSTANCES];
};

float attenuation(float dist)
{
	vec3 att = uniLights[lightIndex].attenuation.xyz;
	return 1.0 / (att.x + dist * (att.y + dist * att.z));
}

vec3 lightDirectionalImpl(float shadow)
{
	return uniLightingBrdf(
		uniLights[lightIndex].color.rgb * shadow,
		-uniLights[lightIndex].direction.xyz,
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightPointImpl(float shadow)
{
	vec3 light = uniLights[lightIndex].color.rgb * attenuation(length(uniLights[lightIndex].position.xyz - position));
	return uniLightingBrdf(
		light * shadow,
		normalize(uniLights[lightIndex].position.xyz - position),
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightSpotImpl(float shadow)
{
	vec3 light = uniLights[lightIndex].color.rgb * attenuation(length(uniLights[lightIndex].position.xyz - position));
	vec3 lightSourceToFragmentDirection = normalize(uniLights[lightIndex].position.xyz - position);
	float d = max(dot(-uniLights[lightIndex].direction.xyz, lightSourceToFragmentDirection), 0.0);
	float a = uniLights[lightIndex].color[3]; // angle
	float e = uniLights[lightIndex].attenuation[3]; // exponent
	if (d < a)
		return vec3(0);
	d = pow(d, e);
	return uniLightingBrdf(
		light * d * shadow,
		lightSourceToFragmentDirection,
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightAmbientImpl()
{
	return albedo * (uniAmbientLight.rgb + emissive);
}
