#include <cmath>

#include "hsluv.h"

#include <cage-core/color.h>
#include <cage-core/math.h>

namespace cage
{
	namespace
	{
		void float2rgbe(uint8 rgbe[4], float red, float green, float blue)
		{
			float v = red;
			if (green > v)
				v = green;
			if (blue > v)
				v = blue;
			if (v < 1e-32)
				rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
			else
			{
				int e = 0;
				v = frexp(v, &e) * 256.0f / v;
				rgbe[0] = (uint8)(red * v);
				rgbe[1] = (uint8)(green * v);
				rgbe[2] = (uint8)(blue * v);
				rgbe[3] = (uint8)(e + 128);
			}
		}

		void rgbe2float(float &red, float &green, float &blue, const uint8 rgbe[4])
		{
			if (rgbe[3])
			{
				const float f = ldexp(1.0f, rgbe[3] - (int)(128 + 8));
				red = rgbe[0] * f;
				green = rgbe[1] * f;
				blue = rgbe[2] * f;
			}
			else
				red = green = blue = 0;
		}

		union CharToInt
		{
			struct
			{
				uint8 rgbe[4];
			};
			uint32 val;
		};

		bool colorInRange(const Vec3 &color)
		{
			return saturate(color) == color;
		}
	}

	uint32 colorRgbToRgbe(const Vec3 &color)
	{
		CAGE_ASSERT(colorInRange(color));
		CharToInt hlp;
		float2rgbe(hlp.rgbe, color.data[0].value, color.data[1].value, color.data[2].value);
		return hlp.val;
	}

	Vec3 colorRgbeToRgb(uint32 color)
	{
		CharToInt hlp;
		hlp.val = color;
		Vec3 res;
		rgbe2float(res.data[0].value, res.data[1].value, res.data[2].value, hlp.rgbe);
		CAGE_ASSERT(colorInRange(res));
		return res;
	}

	Vec3 colorRgbToHsv(const Vec3 &inColor)
	{
		CAGE_ASSERT(colorInRange(inColor));
		Vec3 outColor;
		Real minColor = inColor[0] < inColor[1] ? inColor[0] : inColor[1];
		minColor = minColor < inColor[2] ? minColor : inColor[2];
		Real maxColor = inColor[0] > inColor[1] ? inColor[0] : inColor[1];
		maxColor = maxColor > inColor[2] ? maxColor : inColor[2];
		outColor[2] = maxColor;
		Real delta = maxColor - minColor;
		if (delta < 0.00001)
			return outColor;
		if (maxColor > 0)
			outColor[1] = (delta / maxColor);
		else
			return outColor;
		if (inColor[0] >= maxColor)
			outColor[0] = (inColor[1] - inColor[2]) / delta;
		else if (inColor[1] >= maxColor)
			outColor[0] = 2 + (inColor[2] - inColor[0]) / delta;
		else
			outColor[0] = 4 + (inColor[0] - inColor[1]) / delta;
		outColor[0] *= 60.f / 360.f;
		if (outColor[0] < 0)
			outColor[0] += 1;
		return outColor;
	}

	Vec3 colorHsvToRgb(const Vec3 &inColor)
	{
		CAGE_ASSERT(colorInRange(inColor));
		Vec3 outColor;
		if (inColor[1] <= 0)
		{
			outColor[0] = inColor[2];
			outColor[1] = inColor[2];
			outColor[2] = inColor[2];
			CAGE_ASSERT(colorInRange(outColor));
			return outColor;
		}
		Real hh = inColor[0];
		if (hh >= 1)
			hh = 0;
		hh /= 60.f / 360.f;
		uint32 i = numeric_cast<uint32>(hh);
		Real ff = hh - i;
		Real p = inColor[2] * (1 - inColor[1]);
		Real q = inColor[2] * (1 - (inColor[1] * ff));
		Real t = inColor[2] * (1 - (inColor[1] * (1 - ff)));
		switch (i)
		{
			case 0:
				outColor[0] = inColor[2];
				outColor[1] = t;
				outColor[2] = p;
				break;
			case 1:
				outColor[0] = q;
				outColor[1] = inColor[2];
				outColor[2] = p;
				break;
			case 2:
				outColor[0] = p;
				outColor[1] = inColor[2];
				outColor[2] = t;
				break;
			case 3:
				outColor[0] = p;
				outColor[1] = q;
				outColor[2] = inColor[2];
				break;
			case 4:
				outColor[0] = t;
				outColor[1] = p;
				outColor[2] = inColor[2];
				break;
			case 5:
			default:
				outColor[0] = inColor[2];
				outColor[1] = p;
				outColor[2] = q;
				break;
		}
		CAGE_ASSERT(colorInRange(outColor));
		return outColor;
	}

	Vec3 colorRgbToHsluv(const Vec3 &rgb)
	{
		CAGE_ASSERT(colorInRange(rgb));
		double h, s, l;
		const double r = rgb[0].value, g = rgb[1].value, b = rgb[2].value;
		rgb2hsluv(r, g, b, &h, &s, &l);
		const Vec3 res = Vec3(h / 360, s / 100, l / 100);
		CAGE_ASSERT(colorInRange(res));
		return res;
	}

	Vec3 colorHsluvToRgb(const Vec3 &hsluv)
	{
		CAGE_ASSERT(colorInRange(hsluv));
		const double h = hsluv[0].value * 360, s = hsluv[1].value * 100, l = hsluv[2].value * 100;
		double r, g, b;
		hsluv2rgb(h, s, l, &r, &g, &b);
		return saturate(Vec3(r, g, b)); // the library may return values outside valid range, so we need to saturate here
	}

	Vec3 colorValueToHeatmapRgb(Real inValue)
	{
		Real value = 4.0f * (1.0f - inValue);
		value = clamp(value, 0, 4);
		const int band = int(value.value);
		value -= band;
		Vec3 result;
		switch (band)
		{
			case 0:
				result[0] = 1;
				result[1] = value;
				result[2] = 0;
				break;
			case 1:
				result[0] = 1.0f - value;
				result[1] = 1;
				result[2] = 0;
				break;
			case 2:
				result[0] = 0;
				result[1] = 1;
				result[2] = value;
				break;
			case 3:
				result[0] = 0;
				result[1] = 1.0f - value;
				result[2] = 1;
				break;
			default:
				result[0] = value;
				result[1] = 0;
				result[2] = 1;
				break;
		}
		return result;
	}

	Vec3 colorGammaToLinear(const Vec3 &rgb)
	{
		CAGE_ASSERT(colorInRange(rgb));
		const Vec3 c2 = rgb * rgb;
		const Vec3 c3 = c2 * rgb;
		return 0.755 * c2 + 0.245 * c3;
	}

	Vec3 colorGammaToLinear(const Vec3 &rgb, Real gamma)
	{
		CAGE_ASSERT(colorInRange(rgb));
		CAGE_ASSERT(gamma > 0);
		return Vec3(pow(rgb[0], gamma), pow(rgb[1], gamma), pow(rgb[2], gamma));
	}

	Vec3 colorLinearToGamma(const Vec3 &rgb, Real gamma)
	{
		CAGE_ASSERT(colorInRange(rgb));
		CAGE_ASSERT(gamma > 0);
		return colorGammaToLinear(rgb, 1 / gamma);
	}

	Real distanceColor(const Vec3 &rgb1, const Vec3 &rgb2)
	{
		CAGE_ASSERT(colorInRange(rgb1));
		CAGE_ASSERT(colorInRange(rgb2));
		const Vec3 hsluv1 = colorRgbToHsluv(rgb1);
		const Vec3 hsluv2 = colorRgbToHsluv(rgb2);
		const Real h1 = hsluv1[0];
		const Real h2 = hsluv2[0];
		const Real s1 = hsluv1[1];
		const Real s2 = hsluv2[1];
		const Real l1 = hsluv1[2];
		const Real l2 = hsluv2[2];
		const Real h = distanceWrap(h1, h2);
		const Real s = abs(s1 - s2);
		const Real l = abs(l1 - l2);
		return length(Vec3(h, s, l));
	}

	Vec3 interpolateColor(const Vec3 &rgb1, const Vec3 &rgb2, Real factor)
	{
		CAGE_ASSERT(colorInRange(rgb1));
		CAGE_ASSERT(colorInRange(rgb2));
		CAGE_ASSERT(saturate(factor) == factor);
		const Vec3 hsluv1 = colorRgbToHsluv(rgb1);
		const Vec3 hsluv2 = colorRgbToHsluv(rgb2);
		const Real h1 = hsluv1[0];
		const Real h2 = hsluv2[0];
		const Real s1 = hsluv1[1];
		const Real s2 = hsluv2[1];
		const Real l1 = hsluv1[2];
		const Real l2 = hsluv2[2];
		const Real h = interpolateWrap(h1, h2, factor);
		const Real s = interpolate(s1, s2, factor);
		const Real l = interpolate(l1, l2, factor);
		const Vec3 hsluv = Vec3(h, s, l);
		const Vec3 rgb = colorHsluvToRgb(hsluv);
		CAGE_ASSERT(colorInRange(rgb));
		return rgb;
	}

	Vec2 colorSpecularToRoughnessMetallic(const Vec3 &specular)
	{
		const Vec3 hsv = colorRgbToHsv(specular);
		return Vec2(hsv[2], hsv[1]);
	}
}
