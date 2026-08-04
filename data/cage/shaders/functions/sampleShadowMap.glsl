
float sampleShadowMap2d(texture2DArray texShadow, vec3 shadowPos, int cascade)
{
	return texture( sampler2DArrayShadow(texShadow, samplerShadows), vec4(shadowPos.xy, float(cascade), shadowPos.z));
}

float sampleShadowMapCube(textureCube texShadow, vec3 shadowPos)
{
	return texture(samplerCubeShadow(texShadow, samplerShadows), vec4(normalize(shadowPos), length(shadowPos)));
}
