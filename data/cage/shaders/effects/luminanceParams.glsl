
layout(std140, binding = CAGE_SHADER_UNIBLOCK_CUSTOMDATA) uniform Luminance
{
	vec4 uniLogRange; // min, max range in log2 space
	vec4 uniAdaptationSpeed; // darker, lighter
	vec4 uniNightParams; // nightOffset, nightDesaturation, nightContrast
	vec4 uniApplyParams; // key, strength
};

const int downscale = 4;
