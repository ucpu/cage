$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;

layout(std140, binding = 2) uniform Sharpening
{
	vec4 params; // strength
};

out vec3 outColor;

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);
	vec3 center = texelFetch(texColor, coord, 0).rgb;
	vec3 up = texelFetch(texColor, coord + ivec2(0, 1), 0).rgb;
	vec3 down = texelFetch(texColor, coord + ivec2(0, -1), 0).rgb;
	vec3 left = texelFetch(texColor, coord + ivec2(-1, 0), 0).rgb;
	vec3 right = texelFetch(texColor, coord + ivec2(1, 0), 0).rgb;
	float strength = params.x;
	vec3 sharpenedColor = center * (1 + 4 * strength) - (up + down + left + right) * strength;
	outColor = clamp(sharpenedColor, 0, 1);
}
