
$include ../shaderConventions.h

$define shader vertex

struct InstanceStruct
{
	vec4 wrld;
	vec4 text;
};
layout(std140, binding = 1) uniform Instances
{
	InstanceStruct instances[512];
};

layout(location = 0) uniform mat4 uniMvp;

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;

out vec2 varUv;

void main()
{
	InstanceStruct inst = instances[gl_InstanceID];
	gl_Position = uniMvp * vec4(inPosition.xy * inst.wrld.zw + inst.wrld.xy, 0.0, 1.0);
	varUv = (inPosition.xy * inst.text.zw + inst.text.xy);
}

$define shader fragment

layout(binding = 0) uniform sampler2D uniTexture;

layout(location = 4) uniform vec3 uniColor;

in vec2 varUv;

out vec4 outColor;

float median(vec3 v)
{
	return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main()
{
$if 0
	vec2 dpdx = dFdx(varUv);
	vec2 dpdy = dFdy(varUv);
	float m = length(vec2(length(dpdx), length(dpdy))) * 0.70710678118654757;
	vec3 sampl = texture(uniTexture, varUv).rgb;
	float opacity = median(sampl) / m * 0.005;
	outColor = vec4(uniColor, opacity);
$end

$if 0
	ivec2 sz = textureSize(uniTexture, 0);
	float dx = dFdx(varUv.x) * sz.x;
	float dy = dFdy(varUv.y) * sz.y;
	float toPixels = 5.0 * inversesqrt(dot(dx, dy));
	vec3 sampl = texture(uniTexture, varUv).rgb;
	float opacity = toPixels * (median(sampl) - 0.5) + 0.5;
	outColor = vec4(uniColor, opacity);
$end

$if 1
	float dist = median(texture(uniTexture, varUv).rgb);
	float edgeWidth = length(vec2(dFdx(dist), dFdy(dist))) * 0.70710678118654757;
	float opacity = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, dist);
	outColor = vec4(uniColor, opacity);
$end
}
