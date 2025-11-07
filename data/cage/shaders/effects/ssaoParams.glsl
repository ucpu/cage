
layout(std430, set = 2, binding = 0) readonly buffer Ssao
{
	mat4 proj;
	mat4 projInv;
	vec4 params; // strength, bias, power, raysLength
	ivec4 iparams; // sampleCount, hashSeed
};
