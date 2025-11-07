
$include poissonDisk4.glsl
$include randomAngle.glsl


float sampleShadowMap2d(texture2DArray texShadow, vec3 shadowPos, int cascade)
{
	vec2 radius = 1.3 / textureSize(sampler2DArray(texShadow, samplerShadows), 0).xy;
	float visibility = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float ra = randomAngle(12000.0, shadowPos);
		mat2 rot = mat2(cos(ra), sin(ra), -sin(ra), cos(ra));
		vec2 samp = rot * poissonDisk4[i] * radius;
		visibility += float(textureLod(sampler2DArray(texShadow, samplerShadows), vec3(shadowPos.xy + samp, float(cascade)), 0).x > shadowPos.z);
	}
	visibility /= 4.0;
	return visibility;
}

float sampleShadowMapCube(textureCube texShadow, vec3 shadowPos)
{
	return float(textureLod(samplerCube(texShadow, samplerShadows), normalize(shadowPos), 0).x > length(shadowPos));
}
