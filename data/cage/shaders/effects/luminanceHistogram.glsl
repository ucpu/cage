
$define shader compute

$include luminanceParams.glsl

layout(local_size_x = 256) in;

layout(rgba16f, binding = 0) uniform readonly writeonly image2D texCollect; // w*h
layout(r32ui, binding = 1) uniform uimage2D texHist; // 256x1
layout(r32f, binding = 2) uniform image2D texAccum; // 1x1

shared uint histogramShared[256];

void main()
{
	uint thisBin = uint(imageLoad(texHist, ivec2(gl_LocalInvocationIndex, 0)).x);
	histogramShared[gl_LocalInvocationIndex] = gl_LocalInvocationIndex == 255 ? 0 : thisBin * gl_LocalInvocationIndex;

	barrier();

	imageStore(texHist, ivec2(gl_LocalInvocationIndex, 0), uvec4(0)); // reset for next frame

	for (uint cutoff = 128; cutoff > 0; cutoff /= 2)
	{
		if (uint(gl_LocalInvocationIndex) < cutoff)
			histogramShared[gl_LocalInvocationIndex] += histogramShared[gl_LocalInvocationIndex + cutoff];
		barrier();
	}

	if (gl_LocalInvocationIndex == 255)
	{
		uvec2 dim = imageSize(texCollect).xy / downscale;
		float pixCount = float(dim[0] * dim[1]);
		float average = float(histogramShared[0] / 255) / (pixCount - thisBin);
		float new = exp2((average * (uniLogRange[1] - uniLogRange[0])) + uniLogRange[0]);
		float old = imageLoad(texAccum, ivec2(0, 0)).x;
		float final = old + (new - old) * (1 - exp(-uniAdaptationSpeed[new > old ? 1 : 0]));
		if (isnan(final) || isinf(final))
			final = 0;
		imageStore(texAccum, ivec2(0, 0), vec4(final, 0, 0, 0));
	}
}
