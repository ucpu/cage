
layout(std140, binding = 2) uniform Ssao
{
	mat4 proj;
	mat4 projInv;
	vec4 params; // strength, bias, power, radius
	ivec4 iparams; // sampleCount, hashSeed
};
