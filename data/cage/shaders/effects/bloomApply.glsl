
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texBloom;

layout(location = 0) uniform int uniLodLevel;

out vec3 outColor;

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord), 0).rgb;
	vec3 bloom = vec3(0);
	vec2 uv = vec2(gl_FragCoord) / textureSize(texColor, 0).xy;
	for (int i = 0; i < uniLodLevel; i++)
		bloom += textureLod(texBloom, uv, i).rgb;
	outColor = color + bloom / uniLodLevel; // additive mixing
}
