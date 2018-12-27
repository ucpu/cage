
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_VELOCITY) uniform sampler2D texVelocity;
out vec4 outColor;

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(texColor, 0));
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;
	vec2 velocity = texture(texVelocity, screenTexCoords).rg;
	float speed = length(velocity / texelSize);
	int nSamples = clamp(int(speed), 1, 8);
	outColor = texture(texColor, screenTexCoords);
	for (int i = 1; i < nSamples; ++i)
	{
		vec2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5);
		outColor += texture(texColor, screenTexCoords + offset);
	}
	outColor /= float(nSamples);
}
