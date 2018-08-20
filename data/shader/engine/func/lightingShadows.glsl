
$if 0

layout(binding = CAGE_SHADER_TEXTURE_SHADOW) uniform sampler2DShadow texShadow2d;
layout(binding = CAGE_SHADER_TEXTURE_SHADOW_CUBE) uniform samplerCubeShadow texShadowCube;

float sampleShadowMap2d(vec3 shadowPos)
{
	return texture(texShadow2d, shadowPos).x;
}

float sampleShadowMapCube(vec3 shadowPos)
{
	return texture(texShadowCube, vec4(normalize(shadowPos), length(shadowPos))).x;
}

$else

layout(binding = CAGE_SHADER_TEXTURE_SHADOW) uniform sampler2D texShadow2d;
layout(binding = CAGE_SHADER_TEXTURE_SHADOW_CUBE) uniform samplerCube texShadowCube;

$if 1
$include poissonDisk.glsl

float randomAngle(float freq, vec3 pos)
{
   float dt = dot(floor(pos * freq), vec3(53.1215, 21.1352, 9.1322));
   return fract(sin(dt) * 2105.2354) * 6.283285;
}
$end

float sampleShadowMap2d(vec3 shadowPos)
{
$if 1
	vec2 radius = 0.8 / textureSize(texShadow2d, 0);
	float visibility = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float ra = randomAngle(12000, shadowPos);
		mat2 rot = mat2(cos(ra), sin(ra), -sin(ra), cos(ra));
		vec2 samp = rot * poissonDisk4[i] * radius;
		visibility += texture(texShadow2d, shadowPos.xy + samp).x > shadowPos.z ? 1 : 0;
	}
	visibility /= 4.0;
	return visibility;
$else
	vec4 t = textureGather(texShadow2d, shadowPos.xy);
	float sum = 0;
	for (int i = 0; i < 4; i++)
		sum += t[i] > shadowPos.z ? 1 : 0;
	return sum * 0.25;
$end
}

float sampleShadowMapCube(vec3 shadowPos)
{
	return texture(texShadowCube, normalize(shadowPos)).x > length(shadowPos) ? 1 : 0;
}

$end
