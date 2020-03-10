#include "main.h"
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/color.h>

#include <initializer_list>

void test(real, real);

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
				vec4 color = s <= 1 ? vec4(colorHsvToRgb(vec3(h, s, 1)), 1) : vec4();
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
		img->empty(400, 300, 4);
		drawCircle(img.get());
		img->encodeFile("images/circle8.png");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->decodeFile("images/circle8.png");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
	}

	{
		CAGE_TESTCASE("circle png 16bit");
		Holder<Image> img = newImage();
		img->empty(400, 300, 4, ImageFormatEnum::U16);
		drawCircle(img.get());
		img->encodeFile("images/circle16.png");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->decodeFile("images/circle16.png");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U16);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->encodeFile("images/circle16_2.png");
	}

	{
		CAGE_TESTCASE("blit 1");
		Holder<Image> img = newImage();
		img->decodeFile("images/circle8.png");
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		Holder<Image> tg = newImage();
		imageBlit(img.get(), tg.get(), 50, 0, 0, 0, 300, 300);
		tg->encodeFile("images/circle300.png");
	}

	{
		CAGE_TESTCASE("blit 2");
		Holder<Image> img = newImage();
		img->decodeFile("images/circle8.png");
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		Holder<Image> tg = newImage();
		tg->empty(400, 400, 4);
		imageBlit(img.get(), tg.get(), 50, 0, 50, 50, 300, 300);
		tg->encodeFile("images/circle400.png");
	}

	{
		CAGE_TESTCASE("convert 8 to 16 bit");
		Holder<Image> img = newImage();
		img->decodeFile("images/circle8.png", m, ImageFormatEnum::U16);
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U16);
		img->encodeFile("images/circle8_to_16.png");
	}

	{
		CAGE_TESTCASE("convert 16 to 8 bit");
		Holder<Image> img = newImage();
		img->decodeFile("images/circle16.png", m, ImageFormatEnum::U8);
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->encodeFile("images/circle16_to_8.png");
	}

	{
		CAGE_TESTCASE("convert to 1 channel");
		Holder<Image> img = newImage();
		img->decodeFile("images/circle8.png", 1);
		CAGE_TEST(img->channels() == 1);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->encodeFile("images/circle_slice.png");
	}

	{
		CAGE_TESTCASE("convert to 3 channel");
		Holder<Image> img = newImage();
		img->decodeFile("images/circle8.png", 3);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		CAGE_TEST(img->channels() == 3);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->encodeFile("images/circle_rgb.png");
	}

	{
		CAGE_TESTCASE("store different channels counts in different formats");
		Holder<Image> img = newImage();
		img->empty(400, 300, 4);
		drawCircle(img.get());
		Holder<Image> tg = newImage();
		for (uint32 ch : { 4, 3, 2, 1 })
		{
			CAGE_TESTCASE(stringizer() + ch);
			img->convert(ch);
			for (string fmt : { ".png", ".jpeg", ".tiff", ".tga" })
			{
				if ((ch == 2 || ch == 4) && fmt == ".jpeg")
					continue; // unsupported
				if (ch == 2 && fmt == ".tga")
					continue; // unsupported
				CAGE_TESTCASE(fmt);
				string name = stringizer() + "images/channels/" + ch + fmt;
				img->encodeFile(name);
				tg->decodeFile(name);
				CAGE_TEST(tg->width() == img->width());
				CAGE_TEST(tg->height() == img->height());
				CAGE_TEST(tg->channels() == img->channels());
				CAGE_TEST(tg->format() == img->format());
				if (fmt != ".jpeg") // jpegs are lossy
				{
					for (uint32 c = 0; c < ch; c++)
						test(tg->value(120, 90, c), img->value(120, 90, c));
				}
			}
		}
	}

	{
		CAGE_TESTCASE("circle tiff float");
		Holder<Image> img = newImage();
		img->empty(400, 300, 4, ImageFormatEnum::Float);
		drawCircle(img.get());
		img->encodeFile("images/circleFloat.tiff");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->decodeFile("images/circleFloat.tiff");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::Float);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->encodeFile("images/circleFloat_2.tiff");
	}

	{
		CAGE_TESTCASE("tiff with 6 channels");
		Holder<Image> img = newImage();
		img->empty(256, 256, 6, ImageFormatEnum::U8);
		for (uint32 y = 0; y < 256; y++)
		{
			for (uint32 x = 0; x < 256; x++)
			{
				for (uint32 c = 0; c < 6; c++)
					img->value(x, y, c, ((x + y + c * 10) % 256) / 255.f);
			}
		}
		img->encodeFile("images/6_channels.tiff");
		img = newImage();
		img->decodeFile("images/6_channels.tiff");
		CAGE_TEST(img->width() == 256);
		CAGE_TEST(img->height() == 256);
		CAGE_TEST(img->channels() == 6);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->encodeFile("images/6_channels_2.tiff");
	}

	// todo resizing
}
