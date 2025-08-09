$include ../shaderConventions.h

$define shader vertex

struct InstanceStruct
{
	vec4 wrld;
	vec4 text;
};
layout(std140, binding = CAGE_SHADER_UNIBLOCK_CUSTOMDATA) uniform Instances
{
	InstanceStruct instances[512];
};

layout(binding = 0) uniform sampler2D uniTexture;
layout(location = 0) uniform mat4 uniMvp;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;

out vec2 varUv;
flat out float varPxRange;

void main()
{
	InstanceStruct inst = instances[gl_InstanceID];
	gl_Position = uniMvp * vec4(inPosition.xy * inst.wrld.zw + inst.wrld.xy, 0, 1);
	varUv = inPosition.xy * inst.text.zw + inst.text.xy;
	varPxRange = 6 * inst.wrld.w / (inst.text.w * float(textureSize(uniTexture, 0).y));
}

$define shader fragment

layout(binding = 0) uniform sampler2D uniTexture;
layout(location = 4) uniform vec3 uniColor;

in vec2 varUv;
out vec4 outColor;
flat in float varPxRange;

float median(vec3 v)
{
	return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main()
{
	float sd = median(texture(uniTexture, varUv).rgb);
	float screenPxDistance = varPxRange * (sd - 0.5);
	float opacity = clamp(screenPxDistance + 0.5, 0, 1);
	outColor = vec4(uniColor, opacity);
}
