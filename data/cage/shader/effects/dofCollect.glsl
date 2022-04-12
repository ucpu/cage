
$include vertex.glsl

$define shader fragment

$include dofParams.glsl

layout(location = 0) uniform int uniPass;

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texDepth;

out vec3 outDof;

const int downscale = 3;

void main()
{
	vec2 texelSize = float(downscale) / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = vec3(0.0);
	int cnt = 0;
	for (int y = 0; y < downscale; y++)
		for (int x = 0; x < downscale; x++)
			color += texelFetch(texColor, ivec2(gl_FragCoord) * downscale + ivec2(x, y) - downscale / 2, 0).rgb;
	color /= float(downscale * downscale);
	float depth = textureLod(texDepth, uv, 0).x;
	float contribs[2];
	dofContribution(uv, depth, contribs[0], contribs[1]);
	outDof = color * contribs[uniPass];
}
