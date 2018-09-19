#include "main.h"
#include <cage-core/math.h>
#include <cage-core/png.h>
#include <cage-core/color.h>
#include <cage-core/noise.h>

namespace
{
	void drawCircle(pngImageClass *png)
	{
		uint32 w = png->width(), h = png->height(), channels = png->channels();
		vec2 center = vec2(w, h) * 0.5;
		real radiusMax = min(w, h) * 0.5;
		for (real r = 10; r < radiusMax; r += 1)
		{
			for (rads a; a < rads::Full; a += degs(1))
			{
				vec2 p = center + vec2(sin(a), cos(a)) * r;
				vec3 color = convertHsvToRgb(vec3(real(a) / real::TwoPi, 1, r / radiusMax));
				for (uint32 i = 0; i < channels; i++)
					png->value(numeric_cast<uint32>(p[0]), numeric_cast<uint32>(p[1]), i, color[i].value);
			}
		}
	}
}

void testPng()
{
	CAGE_TESTCASE("png");
	{
		CAGE_TESTCASE("circle");
		holder<pngImageClass> png = newPngImage();
		png->empty(400, 300, 3);
		drawCircle(png.get());
		png->encodeFile("testdir/circle.png");
		CAGE_TEST(png->bufferSize() == 400 * 300 * 3);
		png = newPngImage();
		CAGE_TEST(png->bufferSize() == 0);
		png->decodeFile("testdir/circle.png");
		CAGE_TEST(png->channels() == 3);
		CAGE_TEST(png->width() == 400);
		CAGE_TEST(png->height() == 300);
		CAGE_TEST(png->bufferSize() == 400 * 300 * 3);
	}
}
