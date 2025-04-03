
$include ../shaderConventions.h

$define shader compute

$include luminanceParams.glsl

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba16f, binding = 0) uniform readonly image2D texCollect; // w*h
layout(r32ui, binding = 1) uniform uimage2D texHist; // 256x1

shared uint histogramShared[256];

float meteringMaskFactor(vec2 uv)
{
	// normalize so that the average mask value over the whole screen is (approximately) one
	// https://www.wolframalpha.com/input/?i=integrate+2.1786+*+%281+-+1.414+*+%28%28x+-+0.5%29%5E2+%2B+%28y+-+0.5%29%5E2%29%5E0.5%29+%3B+x+%3D+0+..+1%3B+y+%3D+0+..+1
	return 2.1786 * (1 - 1.414 * length(uv - 0.5));
}

uint colorToBin(vec3 color)
{
	float a = log2(dot(color, vec3(0.2125, 0.7154, 0.0721)) + 1e-5);
	if (a < uniLogRange[0] || a > uniLogRange[1])
		return 255;
	float b = clamp((a - uniLogRange[0]) / (uniLogRange[1] - uniLogRange[0]), 0.0, 1.0);
	return uint(b * 255);
}

void main()
{
	histogramShared[gl_LocalInvocationIndex] = 0;

	barrier();

	uvec2 dim = imageSize(texCollect).xy / downscale;
	if (gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y)
	{
		vec3 color= imageLoad(texCollect, ivec2(gl_GlobalInvocationID.xy) * downscale).xyz;
		float mask = meteringMaskFactor(vec2(gl_GlobalInvocationID.xy) / vec2(dim));
		uint bin = colorToBin(color * mask);
		atomicAdd(histogramShared[bin], 1);
	}

	barrier();

	imageAtomicAdd(texHist, ivec2(gl_LocalInvocationIndex, 0), histogramShared[gl_LocalInvocationIndex]);
}
