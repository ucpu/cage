
$include ../shaderConventions.h

$define shader vertex

$include ../functions/common.glsl

layout(location = 0) uniform vec4 pos;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
out vec2 varUv;

void main()
{
	gl_Position.z = 0;
	gl_Position.w = 1;
	gl_Position.xy = pos.xy + (pos.zw - pos.xy) * inUv;
	varUv = inUv;
}

$define shader fragment

$include ../functions/common.glsl
$include ../functions/hsvToRgb.glsl

$if inputSpec ^ 0 = F
	layout(location = 1) uniform vec3  uniColor;
$else
$if inputSpec ^ 0 = H
$else
	layout(location = 1) uniform float uniHue;
$end
$end
in vec2 varUv;
out vec4 outColor;

void main()
{
$if inputSpec ^ 0 = F
	outColor = vec4(uniColor, 1);
$else
$if inputSpec ^ 0 = H
	outColor = vec4(hsvToRgb(vec3(varUv.x, 1, 1)), 1);
$else
	outColor = vec4(hsvToRgb(vec3(uniHue, varUv)), 1);
$end
$end
}
