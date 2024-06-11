$include ../shaderConventions.h

$define shader vertex

struct InstanceStruct
{
	vec4 wrld;
	vec4 text;
};
layout(std140, binding = 2) uniform Instances
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
	gl_Position = uniMvp * vec4(inPosition.xy * inst.wrld.zw + inst.wrld.xy, 0, 1);
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
	float sd = median(texture(uniTexture, varUv).rgb);
	float edgeWidth = fwidth(sd);
	float opacity = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, sd);
	outColor = vec4(uniColor, opacity);
}
