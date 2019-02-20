
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(location = 0) uniform vec4 uniBloomParams; // threshold

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;

out vec3 outBloom;

void main()
{
	vec2 texelSize = float(CAGE_SHADER_BLOOM_DOWNSCALE) / textureSize(texColor, 0).xy;
	vec3 c = vec3(0.0);
	for (int y = 0; y < CAGE_SHADER_BLOOM_DOWNSCALE; y++)
		for (int x = 0; x < CAGE_SHADER_BLOOM_DOWNSCALE; x++)
			c += textureLod(texColor, (gl_FragCoord.xy + vec2(x, y) - CAGE_SHADER_BLOOM_DOWNSCALE * 0.5) * texelSize, 0).rgb;
	c /= float(CAGE_SHADER_BLOOM_DOWNSCALE * CAGE_SHADER_BLOOM_DOWNSCALE);
	float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
	if (l > uniBloomParams[0])
		outBloom = c;
	else
		outBloom = vec3(0.0);
}
