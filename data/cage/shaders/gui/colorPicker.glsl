
$include ../functions/common.glsl
$include ../functions/hsvToRgb.glsl

layout(std430, set = 2, binding = 0) readonly buffer Global
{
	vec4 uniPos;
	vec4 uniColorAndHue; // rgb, hue
};


layout(location = 0) varying vec2 varUv;


$define shader vertex

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec2 inUv;

void main()
{
	gl_Position.z = 0;
	gl_Position.w = 1;
	gl_Position.xy = uniPos.xy + (uniPos.zw - uniPos.xy) * inPosition.xy;
	varUv = vec2(inUv.x, 1 - inUv.y);
}


$define shader fragment

layout(location = 0) out vec4 outColor;

void main()
{
$if inputSpec ^ 0 = F
	outColor = vec4(uniColorAndHue.rgb, 1);
$else
$if inputSpec ^ 0 = H
	outColor = vec4(hsvToRgb(vec3(varUv.x, 1, 1)), 1);
$else
	outColor = vec4(hsvToRgb(vec3(uniColorAndHue.a, varUv)), 1);
$end
$end
}
