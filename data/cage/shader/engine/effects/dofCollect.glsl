
$include ../../shaderConventions.h
$include vertex.glsl

$define shader fragment

$include dofParams.glsl

layout(location = 0) uniform int uniPass;

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;

out vec3 outDof;

void main()
{
	vec2 texelSize = float(CAGE_SHADER_BLOOM_DOWNSCALE) / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = textureLod(texColor, uv, 0).rgb;
	float depth = textureLod(texDepth, uv, 0).x;
	float contribs[2];
	dofContribution(uv, depth, contribs[0], contribs[1]);
	outDof = color * contribs[uniPass];
}
