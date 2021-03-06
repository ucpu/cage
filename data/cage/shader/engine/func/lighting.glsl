
$include lightingShadows.glsl
$include lightingImpl.glsl

vec4 shadowSamplingPosition4()
{
	float normalOffsetScale = uniLights[lightIndex].direction[3];
	vec3 p3 = position + normal * normalOffsetScale;
	return uniLights[lightIndex].shadowMat * vec4(p3, 1.0);
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
