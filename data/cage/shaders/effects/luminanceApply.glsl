
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

$include luminanceParams.glsl

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texLuminance;

out vec3 outColor;

vec3 desaturate(vec3 color, float strength)
{
	float i = dot(color, vec3(0.299, 0.587, 0.114)); // gray intensity
	return mix(color, vec3(i), strength);
}

vec3 contrast(vec3 color, float strength)
{
	return max((color - 0.5) * (1 + strength) + 0.5, 0.0);
}

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).xyz;
	float avgLuminance = texelFetch(texLuminance, ivec2(0), 0).x;
	vec3 c = color * uniApplyParams[0] / avgLuminance;
	float night = -log(avgLuminance) - uniNightParams[0];
	c = desaturate(c, clamp(night * uniNightParams[1], 0.0, 1.0));
	c = contrast(c, clamp(night * uniNightParams[2], 0.0, 1.0));
	outColor = mix(color, c, uniApplyParams[1]);
}
