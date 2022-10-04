
$include vertex.glsl

$define shader fragment

$include luminanceParams.glsl

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texLuminance;

out vec3 outColor;

vec3 desaturate(vec3 color, float strength)
{
	float i = dot(color, vec3(0.3, 0.59, 0.11)); // gray intensity
	return mix(color, vec3(i), strength);
}

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).xyz;
	float avgLuminance = texelFetch(texLuminance, ivec2(0), 0).x;
	vec3 c = color * uniApplyParams[0] / avgLuminance;
	float night = clamp((-log(avgLuminance) - uniNightParams[0]) * uniNightParams[1], 0.0, 1.0);
	c = desaturate(c, night);
	outColor = mix(color, c, uniApplyParams[1]);
}
