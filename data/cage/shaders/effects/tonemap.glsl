
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;

layout(std140, binding = CAGE_SHADER_UNIBLOCK_CUSTOMDATA) uniform Tonemap
{
	vec4 params; // gamma, tonemapEnabled
};

out vec3 outColor;

// https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
vec3 neutralToneMapping(vec3 color)
{
	const float startCompression = 0.8 - 0.04;
	const float desaturation = 0.15;
	float x = min(color.r, min(color.g, color.b));
	float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
	color -= offset;
	float peak = max(color.r, max(color.g, color.b));
	if (peak < startCompression) return color;
	const float d = 1. - startCompression;
	float newPeak = 1. - d * d / (peak + d - startCompression);
	color *= newPeak / peak;
	float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
	return mix(color, newPeak * vec3(1, 1, 1), g);
}

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).xyz;

	// tone mapping
	if (params[1] > 0.5)
		color = neutralToneMapping(color);

	// gamma correction
	color = pow(color, vec3(params[0]));

	outColor = color;
}
