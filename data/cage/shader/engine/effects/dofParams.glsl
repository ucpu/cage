
layout(std140, binding = CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES) uniform Dof
{
	mat4 projInv;
	vec4 dofNear; // near, far
	vec4 dofFar; // near, far
};

vec3 s2w(vec2 p, float d)
{
	vec4 p4 = vec4(p, d, 1.0);
	p4 = projInv * p4;
	return p4.xyz / p4.w;
}

float interpFactor(float a, float b, float d)
{
	if (a < b)
		return smoothstep(a, b, d);
	return d < a ? 0.0 : 1.0;
}

void dofContribution(vec2 uv, float depth, out float near, out float far)
{
	vec3 wp = s2w(uv * 2.0 - 1.0, depth * 2.0 - 1.0);
	float d = length(wp);
	near = 1.0 - interpFactor(dofNear[0], dofNear[1], d);
	far = interpFactor(dofFar[0], dofFar[1], d);
}
