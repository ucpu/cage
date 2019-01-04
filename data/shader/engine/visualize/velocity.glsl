
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(location = 0) uniform vec2 uniScale;
layout(binding = 0) uniform sampler2D texVelocity;
out vec4 outColor;

void main()
{
	vec2 uv = gl_FragCoord.xy * uniScale;
	vec2 v = textureLod(texVelocity, uv, 0).xy;
	float s = length(v);
	outColor = vec4(abs(v) * pow(s * 10.0, 0.2) / s, 0.0, 1.0);
}
