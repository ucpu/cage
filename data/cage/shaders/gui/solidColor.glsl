
$include ../functions/common.glsl

layout(std140, set = 2, binding = 0) uniform Global
{
	vec4 uniPos;
	vec4 uniColor; // linear
};


$define shader vertex

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec2 inUv;

void main()
{
	gl_Position.z = 0;
	gl_Position.w = 1;
	gl_Position.xy = uniPos.xy + (uniPos.zw - uniPos.xy) * inPosition.xy;
}


$define shader fragment

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = uniColor;
}
