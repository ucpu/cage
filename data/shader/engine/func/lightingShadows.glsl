
layout(binding = CAGE_SHADER_TEXTURE_SHADOW) uniform sampler2D texShadow2d;
layout(binding = CAGE_SHADER_TEXTURE_SHADOW_CUBE) uniform samplerCube texShadowCube;

$if 1

$include poissonDisk.glsl

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

$else

float sampleShadowMap2d(vec3 shadowPos)
{
	vec2 res = 1.0 / textureSize(texShadow2d, 0).xy;
	float visibility = 0.0;
	for (float y = -2.0; y < 3.0; y += 2.0)
		for (float x = -2.0; x < 3.0; x += 2.0)
			visibility += dot(vec4(greaterThan(textureGather(texShadow2d, shadowPos.xy + vec2(x, y) * res), vec4(shadowPos.z))), vec4(0.25));
	return visibility / 9.0;
}

$end

float sampleShadowMapCube(vec3 shadowPos)
{
	return float(texture(texShadowCube, normalize(shadowPos)).x > length(shadowPos));
}
