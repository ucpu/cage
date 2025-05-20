
layout(std140, binding = CAGE_SHADER_UNIBLOCK_CUSTOMDATA) uniform Ssao
{
	mat4 proj;
	mat4 projInv;
	vec4 params; // strength, bias, power, raysLength
	ivec4 iparams; // sampleCount, hashSeed
};
