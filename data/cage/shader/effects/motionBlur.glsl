
$include ../shaderConventions.h
$include vertex.glsl

$define shader fragment

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_EFFECTS) uniform sampler2D texVelocity;

out vec4 outColor;

void main()
{
	vec2 texelSize = 1.0 / textureSize(texColor, 0).xy;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;
	vec2 velocity = textureLod(texVelocity, screenTexCoords, 0).xy;
	float speed = length(velocity / texelSize);
	int nSamples = clamp(int(speed), 1, 8);
	outColor = textureLod(texColor, screenTexCoords, 0);
	for (int i = 1; i < nSamples; ++i)
	{
		vec2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5);
		outColor += textureLod(texColor, screenTexCoords + offset, 0);
	}
	outColor /= float(nSamples);
}
