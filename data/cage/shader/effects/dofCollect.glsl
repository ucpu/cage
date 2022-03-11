
$include ../shaderConventions.h
$include vertex.glsl

$define shader fragment

$include dofParams.glsl

layout(location = 0) uniform int uniPass;

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;

out vec3 outDof;

void main()
{
	vec2 texelSize = float(CAGE_SHADER_DOF_DOWNSCALE) / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = vec3(0.0);
	int cnt = 0;
	for (int y = 0; y < CAGE_SHADER_DOF_DOWNSCALE; y++)
		for (int x = 0; x < CAGE_SHADER_DOF_DOWNSCALE; x++)
			color += texelFetch(texColor, ivec2(gl_FragCoord) * CAGE_SHADER_DOF_DOWNSCALE + ivec2(x, y) - CAGE_SHADER_DOF_DOWNSCALE / 2, 0).rgb;
	color /= float(CAGE_SHADER_DOF_DOWNSCALE * CAGE_SHADER_DOF_DOWNSCALE);
	float depth = textureLod(texDepth, uv, 0).x;
	float contribs[2];
	dofContribution(uv, depth, contribs[0], contribs[1]);
	outDof = color * contribs[uniPass];
}
