
$include vertex.glsl

$define shader fragment

layout(std140, set = 2, binding = 0) uniform Params
{
	vec4 uniBloomParams; // threshold, blur passes
};

layout(set = 2, binding = 1) uniform sampler2D texColor;
layout(set = 2, binding = 3) uniform sampler2D texBloom;

layout(location = 0) out vec4 outColor;

void main()
{
	int levels = int(uniBloomParams.y);
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord), 0).rgb;
	vec3 bloom = vec3(0);
	vec2 uv = vec2(gl_FragCoord) / vec2(textureSize(texColor, 0));
	for (int i = 0; i < levels; i++)
		bloom += textureLod(texBloom, uv, i).rgb;
	outColor = vec4(color + bloom / levels, 1); // additive mixing
}
