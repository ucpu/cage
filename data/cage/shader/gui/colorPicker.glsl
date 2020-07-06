
$include ../shaderConventions.h

$define shader vertex

layout(location = 0) uniform vec4 pos;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
out vec2 varUv;

void main()
{
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
	gl_Position.xy = pos.xy + (pos.zw - pos.xy) * inUv;
	varUv = inUv;
}

$define shader fragment

vec3 hsvToRgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

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
	outColor = vec4(uniColor, 1.0);
$else
$if inputSpec ^ 0 = H
	outColor = vec4(hsvToRgb(vec3(varUv.x, 1.0, 1.0)), 1.0);
$else
	outColor = vec4(hsvToRgb(vec3(uniHue, varUv)), 1.0);
$end
$end
}
