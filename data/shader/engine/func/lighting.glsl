
$include lightingShadows.glsl
$include lightingImpl.glsl

vec3 normalOffsetShadows()
{
	return position + normal * 0.2;
}

vec3 lightDirectional()
{
	return lightDirectionalImpl(1);
}

vec3 lightDirectionalShadow()
{
	vec3 shadowPos = vec3(uniLights[lightIndex].shadowMat * vec4(normalOffsetShadows(), 1.0));
	return lightDirectionalImpl(sampleShadowMap2d(shadowPos));
}

vec3 lightPoint()
{
	return lightPointImpl(1);
}

vec3 lightPointShadow()
{
	vec3 shadowPos = vec3(uniLights[lightIndex].shadowMat * vec4(normalOffsetShadows(), 1.0));
	return lightPointImpl(sampleShadowMapCube(shadowPos));
}

vec3 lightSpot()
{
	return lightSpotImpl(1);
}

vec3 lightSpotShadow()
{
	vec4 shadowPos4 = uniLights[lightIndex].shadowMat * vec4(normalOffsetShadows(), 1.0);
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;
	return lightSpotImpl(sampleShadowMap2d(shadowPos));
}

vec3 lightAmbient()
{
	return lightAmbientImpl();
}

vec3 lightType()
{
	switch (uniRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE])
	{
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL: return lightDirectional();
	case CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW: return lightDirectionalShadow();
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINT: return lightPoint();
	case CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW: return lightPointShadow();
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOT: return lightSpot();
	case CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW: return lightSpotShadow();
	case CAGE_SHADER_ROUTINEPROC_LIGHTAMBIENT: return lightAmbient();
	default: return vec3(191.0, 85.0, 236.0) / 255.0;
	}
}
