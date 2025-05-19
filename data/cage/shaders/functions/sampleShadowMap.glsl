
$include poissonDisk4.glsl
$include randomAngle.glsl

float sampleShadowMap2d(sampler2DArray texShadow2d, vec3 shadowPos, int cascade)
{
	// return float(texture(texShadow2d, vec3(shadowPos.xy, float(cascade))).x > shadowPos.z);

	vec2 radius = 1.3 / textureSize(texShadow2d, 0).xy;
	float visibility = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float ra = randomAngle(12000.0, shadowPos);
		mat2 rot = mat2(cos(ra), sin(ra), -sin(ra), cos(ra));
		vec2 samp = rot * poissonDisk4[i] * radius;
		visibility += float(texture(texShadow2d, vec3(shadowPos.xy + samp, float(cascade))).x > shadowPos.z);
	}
	visibility /= 4.0;
	return visibility;
}

float sampleShadowMapCube(samplerCube texShadowCube, vec3 shadowPos)
{
	return float(texture(texShadowCube, normalize(shadowPos)).x > length(shadowPos));
}
