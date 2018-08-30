
$import shaderConventions.h

$define shader vertex

layout(location = 0) uniform vec4 uniPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position.x = inPosition.x < 0.5 ? uniPosition.x : uniPosition.z;
	gl_Position.y = inPosition.y < 0.5 ? uniPosition.y : uniPosition.w;
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
}

$define shader fragment

layout(location = 1) uniform vec4 uniColor;

out vec4 outColor;

void main()
{
	outColor = uniColor;
}
