
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
	lightStruct uniLights[CAGE_SHADER_MAX_RENDER_INSTANCES];
};

float attenuation(float dist, vec3 att)
{
	return 1.0 / (att.x + dist * (att.y + dist * att.z));
}

vec3 lightDirectionalImpl(float shadow)
{
	return lightingBrdf(
		uniLights[lightIndex].color.rgb * shadow,
		-uniLights[lightIndex].direction.xyz,
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightPointImpl(float shadow)
{
	vec3 light = uniLights[lightIndex].color.rgb * attenuation(length(uniLights[lightIndex].position.xyz - position), uniLights[lightIndex].attenuation.xyz);
	return lightingBrdf(
		light * shadow,
		normalize(uniLights[lightIndex].position.xyz - position),
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightSpotImpl(float shadow)
{
	vec3 light = uniLights[lightIndex].color.rgb * attenuation(length(uniLights[lightIndex].position.xyz - position), uniLights[lightIndex].attenuation.xyz);
	vec3 lightSourceToFragmentDirection = normalize(uniLights[lightIndex].position.xyz - position);
	float d = max(dot(-uniLights[lightIndex].direction.xyz, lightSourceToFragmentDirection), 0);
	if (d < uniLights[lightIndex].color[3])
		return vec3(0);
	d = (d - uniLights[lightIndex].color[3]) / (1 - uniLights[lightIndex].color[3]);
	return lightingBrdf(
		light * pow(d, uniLights[lightIndex].attenuation[3]) * shadow,
		lightSourceToFragmentDirection,
		normalize(uniEyePos.xyz - position)
	);
}

vec3 lightAmbientImpl()
{
	return albedo * (uniAmbientLight.rgb + emissive);
}
