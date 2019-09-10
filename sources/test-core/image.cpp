#include "main.h"
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/color.h>
#include <cage-core/noiseFunction.h>

namespace
{
	void drawCircle(image *png)
	{
		uint32 w = png->width(), h = png->height();
		vec2 center = vec2(w, h) * 0.5;
		real radiusMax = min(w, h) * 0.5;
		for (uint32 y = 0; y < h; y++)
		{
			for (uint32 x = 0; x < w; x++)
			{
				real xx = (real(x) - w / 2) / radiusMax;
				real yy = (real(y) - h / 2) / radiusMax;
				real h = real(wrap(atan2(xx, yy)) / rads::Full());
				real s = length(vec2(xx, yy));
				vec3 color = s <= 1 ? colorHsvToRgb(vec3(h, s, 1)) : vec3();
				png->set(x, y, color);
			}
		}
	}
}

void testImage()
{
	CAGE_TESTCASE("image");

	{
		CAGE_TESTCASE("circle 8bit");
		holder<image> png = newImage();
		png->empty(400, 300, 3);
		drawCircle(png.get());
		png->encodeFile("images/circle1.png");
		CAGE_TEST(png->bufferSize() == 400 * 300 * 3);
		png = newImage();
		CAGE_TEST(png->bufferSize() == 0);
		png->decodeFile("images/circle1.png", m, m);
		CAGE_TEST(png->channels() == 3);
		CAGE_TEST(png->bytesPerChannel() == 1);
		CAGE_TEST(png->width() == 400);
		CAGE_TEST(png->height() == 300);
		CAGE_TEST(png->bufferSize() == 400 * 300 * 3);
	}

	{
		CAGE_TESTCASE("circle 16bit");
		holder<image> png = newImage();
		png->empty(400, 300, 3, 2);
		drawCircle(png.get());
		png->encodeFile("images/circle2.png");
		CAGE_TEST(png->bufferSize() == 400 * 300 * 3 * 2);
		png = newImage();
		CAGE_TEST(png->bufferSize() == 0);
		png->decodeFile("images/circle2.png", m, m);
		CAGE_TEST(png->channels() == 3);
		CAGE_TEST(png->bytesPerChannel() == 2);
		CAGE_TEST(png->width() == 400);
		CAGE_TEST(png->height() == 300);
		CAGE_TEST(png->bufferSize() == 400 * 300 * 3 * 2);
		png->encodeFile("images/circle2_2.png");
	}

	// todo conversions
}
