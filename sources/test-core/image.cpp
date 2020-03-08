#include "main.h"
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/color.h>

namespace
{
	void drawCircle(Image *img)
	{
		uint32 w = img->width(), h = img->height();
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
				img->set(x, y, color);
			}
		}
	}
}

void testImage()
{
	CAGE_TESTCASE("image");

	{
		CAGE_TESTCASE("circle png 8bit");
		Holder<Image> img = newImage();
		img->empty(400, 300, 3);
		drawCircle(img.get());
		img->encodeFile("images/circle8.png");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->decodeFile("images/circle8.png");
		CAGE_TEST(img->channels() == 3);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
	}

	{
		CAGE_TESTCASE("circle png 16bit");
		Holder<Image> img = newImage();
		img->empty(400, 300, 3, ImageFormatEnum::U16);
		drawCircle(img.get());
		img->encodeFile("images/circle16.png");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->decodeFile("images/circle16.png");
		CAGE_TEST(img->channels() == 3);
		CAGE_TEST(img->format() == ImageFormatEnum::U16);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->encodeFile("images/circle16_2.png");
	}

	{
		CAGE_TESTCASE("circle jpg");
		Holder<Image> img = newImage();
		img->empty(400, 300, 3);
		drawCircle(img.get());
		img->encodeFile("images/circle.jpg");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->decodeFile("images/circle.jpg");
		CAGE_TEST(img->channels() == 3);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->encodeFile("images/circle_2.jpg");
	}

	// todo blits
	// todo conversions
	// todo resizing
}
