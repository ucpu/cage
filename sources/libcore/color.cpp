#include <cage-core/math.h>
#include <cage-core/color.h>

#include <cmath>
#include "hsluv.h"

namespace cage
{
	namespace
	{
		void float2rgbe(uint8 rgbe[4], float red, float green, float blue)
		{
			float v;
			int e;
			v = red;
			if (green > v)
				v = green;
			if (blue > v)
				v = blue;
			if (v < 1e-32)
			{
				rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
			}
			else
			{
				v = frexp(v, &e) * 256.0f / v;
				rgbe[0] = (uint8)(red * v);
				rgbe[1] = (uint8)(green * v);
				rgbe[2] = (uint8)(blue * v);
				rgbe[3] = (uint8)(e + 128);
			}
		}

		void rgbe2float(float &red, float &green, float &blue, const uint8 rgbe[4])
		{
			float f;
			if (rgbe[3])
			{
				f = ldexp(1.0f, rgbe[3] - (int)(128 + 8));
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

		bool colorInRange(const vec3 &color)
		{
			return clamp(color, 0, 1) == color;
		}
	}

	uint32 colorRgbToRgbe(const vec3 &color)
	{
		CAGE_ASSERT(colorInRange(color));
		CharToInt hlp;
		float2rgbe(hlp.rgbe, color.data[0].value, color.data[1].value, color.data[2].value);
		return hlp.val;
	}

	vec3 colorRgbeToRgb(uint32 color)
	{
		CharToInt hlp;
		hlp.val = color;
		vec3 res;
		rgbe2float(res.data[0].value, res.data[1].value, res.data[2].value, hlp.rgbe);
		return res;
	}

	vec3 colorRgbToHsv(const vec3 &inColor)
	{
		CAGE_ASSERT(colorInRange(inColor));
		vec3 outColor;
		real minColor = inColor[0] < inColor[1] ? inColor[0] : inColor[1];
		minColor = minColor < inColor[2] ? minColor : inColor[2];
		real maxColor = inColor[0] > inColor[1] ? inColor[0] : inColor[1];
		maxColor = maxColor > inColor[2] ? maxColor : inColor[2];
		outColor[2] = maxColor;
		real delta = maxColor - minColor;
		if (delta < 0.00001)
			return outColor;
		if (maxColor > 0)
			outColor[1] = (delta / maxColor);
		else
			return outColor;
		if (inColor[0] >= maxColor)
			outColor[0] = (inColor[1] - inColor[2]) / delta;
		else
			if (inColor[1] >= maxColor)
				outColor[0] = 2 + (inColor[2] - inColor[0]) / delta;
			else
				outColor[0] = 4 + (inColor[0] - inColor[1]) / delta;
		outColor[0] *= 60.f / 360.f;
		if (outColor[0] < 0)
			outColor[0] += 1;
		return outColor;
	}

	vec3 colorHsvToRgb(const vec3 &inColor)
	{
		CAGE_ASSERT(colorInRange(inColor));
		vec3 outColor;
		if (inColor[1] <= 0)
		{
			outColor[0] = inColor[2];
			outColor[1] = inColor[2];
			outColor[2] = inColor[2];
			return outColor;
		}
		real hh = inColor[0];
		if (hh >= 1)
			hh = 0;
		hh /= 60.f / 360.f;
		uint32 i = numeric_cast<uint32>(hh);
		real ff = hh - i;
		real p = inColor[2] * (1 - inColor[1]);
		real q = inColor[2] * (1 - (inColor[1] * ff));
		real t = inColor[2] * (1 - (inColor[1] * (1 - ff)));
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
		return outColor;
	}

	vec3 colorRgbToHsluv(const vec3 &rgb)
	{
		CAGE_ASSERT(colorInRange(rgb));
		double h, s, l;
		double r = rgb[0].value, g = rgb[1].value, b = rgb[2].value;
		rgb2hsluv(r, g, b, &h, &s, &l);
		return vec3(h / 360, s / 100, l / 100);
	}

	vec3 colorHsluvToRgb(const vec3 &hsluv)
	{
		CAGE_ASSERT(colorInRange(hsluv));
		double h = hsluv[0].value * 360, s = hsluv[1].value * 100, l = hsluv[2].value * 100;
		double r, g, b;
		hsluv2rgb(h, s, l, &r, &g, &b);
		return vec3(r, g, b);
	}

	vec3 colorValueToHeatmapRgb(real inValue)
	{
		real value = 4.0f * (1.0f - inValue);
		value = clamp(value, 0, 4);
		int band = int(value.value);
		value -= band;
		vec3 result;
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

	vec3 colorGammaToLinear(const vec3 &rgb)
	{
		CAGE_ASSERT(colorInRange(rgb));
		vec3 c2 = rgb * rgb;
		vec3 c3 = c2 * rgb;
		return 0.755 * c2 + 0.245 * c3;
	}

	vec3 colorGammaToLinear(const vec3 &rgb, real gamma)
	{
		CAGE_ASSERT(colorInRange(rgb));
		CAGE_ASSERT(gamma > 0);
		return vec3(pow(rgb[0], gamma), pow(rgb[1], gamma), pow(rgb[2], gamma));
	}

	vec3 colorLinearToGamma(const vec3 &rgb, real gamma)
	{
		CAGE_ASSERT(colorInRange(rgb));
		CAGE_ASSERT(gamma > 0);
		return colorGammaToLinear(rgb, 1 / gamma);
	}

	real distanceColor(const vec3 &rgb1, const vec3 &rgb2)
	{
		CAGE_ASSERT(colorInRange(rgb1));
		CAGE_ASSERT(colorInRange(rgb2));
		vec3 hsluv1 = colorRgbToHsluv(rgb1);
		vec3 hsluv2 = colorRgbToHsluv(rgb2);
		real h1 = hsluv1[0];
		real h2 = hsluv2[0];
		real s1 = hsluv1[1];
		real s2 = hsluv2[1];
		real l1 = hsluv1[2];
		real l2 = hsluv2[2];
		real h = distanceWrap(h1, h2);
		real s = abs(s1 - s2);
		real l = abs(l1 - l2);
		return length(vec3(h, s, l));
	}

	vec3 interpolateColor(const vec3 &rgb1, const vec3 &rgb2, real factor)
	{
		CAGE_ASSERT(colorInRange(rgb1));
		CAGE_ASSERT(colorInRange(rgb2));
		vec3 hsluv1 = colorRgbToHsluv(rgb1);
		vec3 hsluv2 = colorRgbToHsluv(rgb2);
		real h1 = hsluv1[0];
		real h2 = hsluv2[0];
		real s1 = hsluv1[1];
		real s2 = hsluv2[1];
		real l1 = hsluv1[2];
		real l2 = hsluv2[2];
		real h = interpolateWrap(h1, h2, factor);
		real s = interpolate(s1, s2, factor);
		real l = interpolate(l1, l2, factor);
		vec3 hsluv = vec3(h, s, l);;
		vec3 rgb = colorHsluvToRgb(hsluv);
		return rgb;
	}
}
