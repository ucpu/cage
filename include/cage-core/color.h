#ifndef guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978
#define guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978

namespace cage
{
	// colors are in range 0 .. 1 in all color spaces

	CAGE_API uint32 colorRgbToRgbe(const vec3 &rgb);
	CAGE_API vec3 colorRgbeToRgb(uint32 rgbe);
	CAGE_API vec3 colorRgbToHsv(const vec3 &rgb);
	CAGE_API vec3 colorHsvToRgb(const vec3 &hsv);
	CAGE_API vec3 colorRgbToHsluv(const vec3 &rgb);
	CAGE_API vec3 colorHsluvToRgb(const vec3 &hsluv);

	CAGE_API vec3 colorValueToHeatmapRgb(real value); // 0 < blue < green < red < 1

	CAGE_API vec3 colorGammaToLinear(const vec3 &rgb, real gamma = 2.2);
	CAGE_API vec3 colorLinearToGamma(const vec3 &rgb, real gamma = 2.2);

	CAGE_API real distanceColor(const vec3 &rgb1, const vec3 &rgb2);
	CAGE_API vec3 interpolateColor(const vec3 &rgb1, const vec3 &rgb2, real factor);
}

#endif // guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978
