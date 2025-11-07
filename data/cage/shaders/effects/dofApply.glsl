
$include vertex.glsl

$define shader fragment

layout(std430, set = 2, binding = 0) readonly buffer Params
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
	vec3 wp = s2w(uv * 2.0 - 1.0, depth);
	float d = length(wp);
	near = 1.0 - interpFactor(dofNear[0], dofNear[1], d);
	far = interpFactor(dofFar[0], dofFar[1], d);
}

layout(set = 2, binding = 1) uniform sampler2D texColor;
layout(set = 2, binding = 3) uniform sampler2D texDepth;
layout(set = 2, binding = 5) uniform sampler2D texDof;

layout(location = 0) out vec4 outColor;

void main()
{
	vec2 texelSize = 1.0 / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord), 0).rgb;
	float depth = texelFetch(texDepth, ivec2(gl_FragCoord), 0).x;
	if (depth > 1 - 1e-7)
	{ // skybox
		outColor = vec4(color, 1);
		return;
	}
	vec3 colorDof = textureLod(texDof, uv, 0).rgb;
	float near;
	float far;
	dofContribution(uv, depth, near, far);
	outColor = vec4(color * (1 - near - far) + colorDof * (near + far), 1);
}
