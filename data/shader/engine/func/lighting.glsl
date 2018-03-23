
$include lightingShadows.glsl
$include lightingImpl.glsl

subroutine vec3 lightTypeFunc();

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL) subroutine (lightTypeFunc) vec3 lightDirectional()
{
	return lightDirectionalImpl(1);
}

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW) subroutine (lightTypeFunc) vec3 lightDirectionalShadow()
{
	vec3 shadowPos = vec3(uniLights[lightIndex].shadowMat * vec4(position, 1.0));
	return lightDirectionalImpl(sampleShadowMap2d(shadowPos));
}

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTPOINT) subroutine (lightTypeFunc) vec3 lightPoint()
{
	return lightPointImpl(1);
}

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW) subroutine (lightTypeFunc) vec3 lightPointShadow()
{
	vec3 shadowPos = vec3(uniLights[lightIndex].shadowMat * vec4(position, 1.0));
	return lightPointImpl(sampleShadowMapCube(shadowPos));
}

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTSPOT) subroutine (lightTypeFunc) vec3 lightSpot()
{
	return lightSpotImpl(1);
}

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW) subroutine (lightTypeFunc) vec3 lightSpotShadow()
{
	vec4 shadowPos4 = uniLights[lightIndex].shadowMat * vec4(position, 1.0);
	vec3 shadowPos = shadowPos4.xyz / shadowPos4.w;
	return lightSpotImpl(sampleShadowMap2d(shadowPos));
}

layout(index = CAGE_SHADER_ROUTINEPROC_LIGHTAMBIENT) subroutine (lightTypeFunc) vec3 lightAmbient()
{
	return lightAmbientImpl();
}

layout(location = CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE) subroutine uniform lightTypeFunc uniLightType;
