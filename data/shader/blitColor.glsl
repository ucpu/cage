
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
layout(location = 0) uniform vec4 uniViewport;
out vec2 varUv;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
	varUv = uniViewport.xy + inUv * uniViewport.zw;
}

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;
in vec2 varUv;
out vec4 outColor;

void main()
{
	outColor = vec4(textureLod(texColor, varUv, 0).rgb, 1.0);
}
