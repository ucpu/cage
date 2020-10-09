
struct lightStruct
{
	mat4 shadowMat;
	mat4 mvpMat;
	vec4 color; // + spotAngle
	vec4 position;
	vec4 direction; // + normalOffsetScale
	vec4 attenuation; // + spotExponent
};

layout(std140, binding = CAGE_SHADER_UNIBLOCK_LIGHTS) uniform Lights
{
	lightStruct uniLights[CAGE_SHADER_MAX_INSTANCES];
};
