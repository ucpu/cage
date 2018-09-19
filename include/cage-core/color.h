#ifndef guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978
#define guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978

namespace cage
{
	CAGE_API uint32 convertColorToRgbe(const vec3 &color);
	CAGE_API vec3 convertRgbeToColor(uint32 color);
	CAGE_API vec3 convertRgbToHsv(const vec3 &color);
	CAGE_API vec3 convertHsvToRgb(const vec3 &color);
	CAGE_API vec3 convertToRainbowColor(real value);
}

#endif // guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978
