
layout(std140, set = 2, binding = 0) uniform Global
{
	mat4 uniMvp;
	vec4 uniColor;
};

struct InstanceStruct
{
	vec4 wrld;
	vec4 text;
};
layout(std430, set = 2, binding = 1) readonly buffer Instances
{
	InstanceStruct uniInstances[];
};

layout(location = 0) varying vec2 varUv;
layout(location = 1) varying flat int varInstanceId;


$define shader vertex

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec2 inUv;

void main()
{
	varInstanceId = gl_InstanceIndex;
	InstanceStruct inst = uniInstances[varInstanceId];
	gl_Position = uniMvp * vec4(inPosition.xy * inst.wrld.zw + inst.wrld.xy, 0, 1);
	varUv = inPosition.xy * inst.text.zw + inst.text.xy;
}


$define shader fragment

layout(set = 2, binding = 2) uniform sampler2D uniTexture;

layout(location = 0) out vec4 outColor;

float median(vec3 v)
{
	return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main()
{
	InstanceStruct inst = uniInstances[varInstanceId];
	float pxRange = 6 * inst.wrld.w / (inst.text.w * float(textureSize(uniTexture, 0).y));
	float sd = median(texture(uniTexture, varUv).rgb);
	float screenPxDistance = pxRange * (sd - 0.5);
	float opacity = clamp(screenPxDistance + 0.5, 0, 1);
	outColor = vec4(uniColor.rgb, uniColor.a * opacity);
}
