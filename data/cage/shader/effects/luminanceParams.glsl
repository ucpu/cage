
layout(std140, binding = 0) uniform Luminance
{
	vec4 uniLogRange; // min, max range in log2 space
	vec4 uniAdaptationSpeed; // darker, lighter
	vec4 uniNightParams; // nightOffset, nightDesaturation, nightContrast
	vec4 uniApplyParams; // key, strength
};

const int downscale = 4;
