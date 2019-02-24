
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_EFFECTS) uniform sampler2D texBloom;

layout(location = 0) uniform int uniLodLevel;

out vec3 outColor;

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord), 0).xyz;
	vec3 bloom = vec3(0);
	vec2 uv = vec2(gl_FragCoord) / textureSize(texColor, 0).xy;
	for (int i = 0; i < uniLodLevel; i++)
		bloom += textureLod(texBloom, uv, i).rgb;
	outColor = color + bloom / uniLodLevel; // additive mixing
}
