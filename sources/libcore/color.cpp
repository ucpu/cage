#include <cmath>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/color.h>

namespace cage
{
	namespace
	{
		/* standard conversion from float pixels to rgbe pixels */
		void float2rgbe(unsigned char rgbe[4], float red, float green, float blue)
		{
			float v;
			int e;
			v = red;
			if (green > v) v = green;
			if (blue > v) v = blue;
			if (v < 1e-32)
			{
				rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
			}
			else
			{
				v = frexp(v, &e) * 256.0f / v;
				rgbe[0] = (unsigned char)(red * v);
				rgbe[1] = (unsigned char)(green * v);
				rgbe[2] = (unsigned char)(blue * v);
				rgbe[3] = (unsigned char)(e + 128);
			}
		}

		/* standard conversion from rgbe to float pixels */
		void rgbe2float(float &red, float &green, float &blue, const unsigned char rgbe[4])
		{
			float f;
			if (rgbe[3])
			{   /*nonzero pixel*/
				f = ldexp(1.0f, rgbe[3] - (int)(128 + 8));
				red = rgbe[0] * f;
				green = rgbe[1] * f;
				blue = rgbe[2] * f;
			}
			else
				red = green = blue = 0;
		}

		union chartoint
		{
			struct
			{
				unsigned char rgbe[4];
			};
			uint32 val;
		};
	}

	uint32 convertColorToRgbe(const vec3 &color)
	{
		chartoint hlp;
		float2rgbe(hlp.rgbe, color.data[0].value, color.data[1].value, color.data[2].value);
		return hlp.val;
	}

	vec3 convertRgbeToColor(uint32 color)
	{
		chartoint hlp;
		hlp.val = color;
		vec3 res;
		rgbe2float(res.data[0].value, res.data[1].value, res.data[2].value, hlp.rgbe);
		return res;
	}

	vec3 convertRgbToHsv(const vec3 &inColor)
	{
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

	vec3 convertHsvToRgb(const vec3 &inColor)
	{
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

	vec3 convertToRainbowColor(real inValue)
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
}
