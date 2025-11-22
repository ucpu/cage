
layout(std140, set = 2, binding = 0) uniform Ssao
{
	mat4 proj;
	mat4 projInv;
	vec4 params; // strength, bias, power, raysLength
	ivec4 iparams; // sampleCount, hashSeed
};
