
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;

out vec4 outDof;

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
	outDof = vec4(color, 1);
}
