
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_AMBIENTOCCLUSION) uniform sampler2D texAo;

out vec4 outColor;

layout(location = 0) uniform vec3 uniAoIntensity;

void main()
{
	vec2 texelSize = 1.0 / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	float ao = textureLod(texAo, uv, 0).x;
	vec4 color = textureLod(texColor, uv, 0);
	outColor = vec4(color.rgb - vec3(ao) * uniAoIntensity, 1.0);
}
