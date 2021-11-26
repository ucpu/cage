#ifndef guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978
#define guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978

#include "math.h"

namespace cage
{
	// colors are in range 0 .. 1 in all color spaces

	CAGE_CORE_API uint32 colorRgbToRgbe(const Vec3 &rgb);
	CAGE_CORE_API Vec3 colorRgbeToRgb(uint32 rgbe);
	CAGE_CORE_API Vec3 colorRgbToHsv(const Vec3 &rgb);
	CAGE_CORE_API Vec3 colorHsvToRgb(const Vec3 &hsv);
	CAGE_CORE_API Vec3 colorRgbToHsluv(const Vec3 &rgb);
	CAGE_CORE_API Vec3 colorHsluvToRgb(const Vec3 &hsluv);

	CAGE_CORE_API Vec3 colorValueToHeatmapRgb(Real value); // 0 < blue < green < red < 1

	CAGE_CORE_API Vec3 colorGammaToLinear(const Vec3 &rgb); // fast approximate for gamma = 2.2
	CAGE_CORE_API Vec3 colorGammaToLinear(const Vec3 &rgb, Real gamma);
	CAGE_CORE_API Vec3 colorLinearToGamma(const Vec3 &rgb, Real gamma = 2.2);

	CAGE_CORE_API Real distanceColor(const Vec3 &rgb1, const Vec3 &rgb2);
	CAGE_CORE_API Vec3 interpolateColor(const Vec3 &rgb1, const Vec3 &rgb2, Real factor);

	CAGE_CORE_API Vec2 colorSpecularToRoughnessMetallic(const Vec3 &specular);
}

#endif // guard_colors_h_FE9390CFCE3E4DF2B955C7AF40FD8978
