
layout(std140, binding = CAGE_SHADER_UNIBLOCK_SSAO) uniform Ssao
{
	mat4 viewProj;
	mat4 viewProjInv;
	vec4 params; // strength, bias, power, radius
	vec3 ambientLight;
};
