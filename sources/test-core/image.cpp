#include "main.h"
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/color.h>
#include <cage-core/timer.h>

#include <initializer_list>

void test(Real, Real);
void test(const Vec2 &, const Vec2 &);
void test(const Vec3 &, const Vec3 &);
void test(const Vec4 &, const Vec4 &);

namespace
{
	void drawCircle(Image *img)
	{
		uint32 w = img->width(), h = img->height();
		Vec2 center = Vec2(w, h) * 0.5;
		Real radiusMax = min(w, h) * 0.5;
		for (uint32 y = 0; y < h; y++)
		{
			for (uint32 x = 0; x < w; x++)
			{
				Real xx = (Real(x) - w / 2) / radiusMax;
				Real yy = (Real(y) - h / 2) / radiusMax;
				Real h = Real(wrap(atan2(xx, yy)) / Rads::Full());
				Real s = length(Vec2(xx, yy));
				Vec4 color = s <= 1 ? Vec4(colorHsvToRgb(Vec3(h, s, 1)), 1) : Vec4();
				img->set(x, y, color);
			}
		}
	}

	void drawStripes(Image *img)
	{
		uint32 w = img->width(), h = img->height(), cc = img->channels();
		for (uint32 y = 0; y < h; y++)
		{
			for (uint32 x = 0; x < w; x++)
			{
				for (uint32 c = 0; c <cc; c++)
					img->value(x, y, c, ((x + y + c * 10) % 256) / 255.f);
			}
		}
	}
}

void testImage()
{
	CAGE_TESTCASE("image");

	{
		CAGE_TESTCASE("valid values for different formats");
		Holder<Image> img = newImage();
		{
			img->initialize(1, 1, 1, ImageFormatEnum::U8);
			img->value(0, 0, 0, -2.f);
			test(img->value(0, 0, 0), 0);
			img->value(0, 0, 0, 0.2f);
			test(img->value(0, 0, 0), 0.2);
			img->value(0, 0, 0, 1.55f);
			test(img->value(0, 0, 0), 1.0);
			img->value(0, 0, 0, Real::Nan());
			test(img->value(0, 0, 0), 0.0);
		}
		{
			img->initialize(1, 1, 1, ImageFormatEnum::U16);
			img->value(0, 0, 0, -2.f);
			test(img->value(0, 0, 0), 0);
			img->value(0, 0, 0, 0.2f);
			test(img->value(0, 0, 0), 0.2);
			img->value(0, 0, 0, 1.55f);
			test(img->value(0, 0, 0), 1.0);
			img->value(0, 0, 0, Real::Nan());
			test(img->value(0, 0, 0), 0.0);
		}
		{
			img->initialize(1, 1, 1, ImageFormatEnum::Float);
			img->value(0, 0, 0, -2.f);
			test(img->value(0, 0, 0), -2);
			img->value(0, 0, 0, 0.2f);
			test(img->value(0, 0, 0), 0.2);
			img->value(0, 0, 0, 1.55f);
			test(img->value(0, 0, 0), 1.55);
			img->value(0, 0, 0, Real::Nan());
			CAGE_TEST(!valid(img->value(0, 0, 0)));
		}
	}

	{
		CAGE_TESTCASE("fill");
		Holder<Image> img = newImage();
		img->initialize(150, 40, 1);
		imageFill(+img, 0.4);
		test(img->get1(42, 13), 0.4);
		img->initialize(150, 40, 2);
		imageFill(+img, Vec2(0.1, 0.2));
		test(img->get2(42, 13) / 100, Vec2(0.1, 0.2) / 100);
		img->initialize(150, 40, 3);
		imageFill(+img, Vec3(0.1, 0.2, 0.3));
		test(img->get3(42, 13) / 100, Vec3(0.1, 0.2, 0.3) / 100);
		img->initialize(150, 40, 4);
		imageFill(+img, Vec4(0.1, 0.2, 0.3, 0.4));
		test(img->get4(42, 13) / 100, Vec4(0.1, 0.2, 0.3, 0.4) / 100);
		imageFill(+img, 2, 0.5);
		test(img->get4(42, 13) / 100, Vec4(0.1, 0.2, 0.5, 0.4) / 100);
	}

	{
		CAGE_TESTCASE("circle png 8bit");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4);
		drawCircle(+img);
		img->exportFile("images/formats/circle8.png");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->importFile("images/formats/circle8.png");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
	}

	{
		CAGE_TESTCASE("circle png 16bit");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4, ImageFormatEnum::U16);
		drawCircle(+img);
		img->exportFile("images/formats/circle16.png");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->importFile("images/formats/circle16.png");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U16);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->exportFile("images/formats/circle16_2.png");
	}

	{
		CAGE_TESTCASE("circle dds");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4);
		drawCircle(+img);
		img->exportFile("images/formats/circle.dds");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->importFile("images/formats/circle.dds");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
	}

	{
		CAGE_TESTCASE("blit 1");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		Holder<Image> tg = newImage();
		imageBlit(+img, tg.get(), 50, 0, 0, 0, 300, 300);
		tg->exportFile("images/formats/circle300.png");
	}

	{
		CAGE_TESTCASE("blit 2");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		Holder<Image> tg = newImage();
		tg->initialize(400, 400, 4);
		imageBlit(+img, tg.get(), 50, 0, 50, 50, 300, 300);
		tg->exportFile("images/formats/circle400.png");
	}

	{
		CAGE_TESTCASE("convert 8 to 16 bit");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png", m, ImageFormatEnum::U16);
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U16);
		img->exportFile("images/formats/circle8_to_16.png");
	}

	{
		CAGE_TESTCASE("convert 16 to 8 bit");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle16.png", m, ImageFormatEnum::U8);
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->exportFile("images/formats/circle16_to_8.png");
	}

	{
		CAGE_TESTCASE("convert to 1 channel");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png", 1);
		CAGE_TEST(img->channels() == 1);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->exportFile("images/formats/circle_slice.png");
	}

	{
		CAGE_TESTCASE("convert to 3 channel");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png", 3);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		CAGE_TEST(img->channels() == 3);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->exportFile("images/formats/circle_rgb.png");
	}

	{
		CAGE_TESTCASE("linearize (degamma) and back");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		imageConvert(+img, GammaSpaceEnum::Linear);
		img->exportFile("images/formats/circle_linear.png");
		imageConvert(+img, GammaSpaceEnum::Gamma);
		img->exportFile("images/formats/circle_linear_2.png");
	}

	{
		CAGE_TESTCASE("premultiply alpha and back");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		imageConvert(+img, AlphaModeEnum::PremultipliedOpacity);
		img->exportFile("images/formats/circle_premultiplied.png");
		imageConvert(+img, AlphaModeEnum::Opacity);
		img->exportFile("images/formats/circle_premultiplied_2.png");
	}

	{
		CAGE_TESTCASE("upscale");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		imageResize(+img, 600, 450, true);
		img->exportFile("images/formats/circle_upscaled_colorconfig.png");
		img->importFile("images/formats/circle8.png");
		imageResize(+img, 600, 450, false);
		img->exportFile("images/formats/circle_upscaled_raw.png");
	}

	{
		CAGE_TESTCASE("downscale");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		imageResize(+img, 200, 150, true);
		img->exportFile("images/formats/circle_downscaled_colorconfig.png");
		img->importFile("images/formats/circle8.png");
		imageResize(+img, 200, 150, false);
		img->exportFile("images/formats/circle_downscaled_raw.png");
	}

	{
		CAGE_TESTCASE("stretch");
		Holder<Image> img = newImage();
		img->importFile("images/formats/circle8.png");
		imageResize(+img, 300, 500);
		img->exportFile("images/formats/circle_stretched.png");
	}

	{
		CAGE_TESTCASE("store different channels counts in different formats");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4);
		drawCircle(+img);
		for (uint32 ch : { 4, 3, 2, 1 })
		{
			CAGE_TESTCASE(Stringizer() + ch);
			imageConvert(+img, ch);
			for (const String &fmt : { ".png", ".jpeg", ".tiff", ".tga", ".psd" })
			{
				if ((ch == 2 || ch == 4) && fmt == ".jpeg")
					continue; // unsupported
				if (ch == 2 && fmt == ".tga")
					continue; // unsupported
				CAGE_TESTCASE(fmt);
				const String name = Stringizer() + "images/channels/" + ch + fmt;
				img->exportFile(name);
				Holder<Image> tg = newImage();
				tg->importFile(name);
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
		img->initialize(400, 300, 4, ImageFormatEnum::Float);
		drawCircle(+img);
		img->exportFile("images/formats/circleFloat.tiff");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->importFile("images/formats/circleFloat.tiff");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::Float);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->exportFile("images/formats/circleFloat_2.tiff");
	}

	{
		CAGE_TESTCASE("tiff with 6 channels");
		Holder<Image> img = newImage();
		img->initialize(256, 256, 6, ImageFormatEnum::U8);
		drawStripes(+img);
		img->exportFile("images/formats/6_channels.tiff");
		img = newImage();
		img->importFile("images/formats/6_channels.tiff");
		CAGE_TEST(img->width() == 256);
		CAGE_TEST(img->height() == 256);
		CAGE_TEST(img->channels() == 6);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->exportFile("images/formats/6_channels_2.tiff");
	}

	{
		CAGE_TESTCASE("psd with 6 channels");
		Holder<Image> img = newImage();
		img->initialize(256, 256, 6, ImageFormatEnum::U8);
		drawStripes(+img);
		img->exportFile("images/formats/6_channels.psd");
		img = newImage();
		img->importFile("images/formats/6_channels.psd");
		CAGE_TEST(img->width() == 256);
		CAGE_TEST(img->height() == 256);
		CAGE_TEST(img->channels() == 6);
		CAGE_TEST(img->format() == ImageFormatEnum::U8);
		img->exportFile("images/formats/6_channels_2.psd");
	}

	{
		CAGE_TESTCASE("circle exr");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4, ImageFormatEnum::Float);
		drawCircle(+img);
		img->exportFile("images/formats/circleFloat.exr");
		CAGE_TEST(img->width() == 400);
		img = newImage();
		CAGE_TEST(img->width() == 0);
		img->importFile("images/formats/circleFloat.exr");
		CAGE_TEST(img->channels() == 4);
		CAGE_TEST(img->format() == ImageFormatEnum::Float);
		CAGE_TEST(img->width() == 400);
		CAGE_TEST(img->height() == 300);
		img->exportFile("images/formats/circleFloat_2.exr");
	}

	{
		CAGE_TESTCASE("dilation without transparency");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4);
		drawCircle(+img);
		imageConvert(+img, 3);
		imageDilation(+img, 1);
		img->exportFile("images/dilation/3_1.png");
		CAGE_TEST(img->value(50 - 1, 150, 0) < 0.1);
		CAGE_TEST(img->value(50 - 1, 150, 1) > 0.9);
		CAGE_TEST(img->value(50 - 1, 150, 2) > 0.9);
		img->initialize(400, 300, 4);
		drawCircle(+img);
		imageConvert(+img, 3);
		{
			Holder<Timer> timer = newTimer();
			imageDilation(+img, 5);
			CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "duration: " + timer->duration());
		}
		img->exportFile("images/dilation/3_5.png");
		CAGE_TEST(img->value(50 - 5, 150, 0) < 0.1);
		CAGE_TEST(img->value(50 - 5, 150, 1) > 0.9);
		CAGE_TEST(img->value(50 - 5, 150, 2) > 0.9);
	}

	{
		CAGE_TESTCASE("dilation with transparency");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4);
		drawCircle(+img);
		imageDilation(+img, 1);
		img->exportFile("images/dilation/4_1.png");
		CAGE_TEST(img->value(50 - 1, 150, 0) < 0.1);
		CAGE_TEST(img->value(50 - 1, 150, 1) > 0.9);
		CAGE_TEST(img->value(50 - 1, 150, 2) > 0.9);
		CAGE_TEST(img->value(50 - 1, 150, 3) > 0.9);
		img->initialize(400, 300, 4);
		drawCircle(+img);
		{
			Holder<Timer> timer = newTimer();
			imageDilation(+img, 5);
			CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "duration: " + timer->duration());
		}
		img->exportFile("images/dilation/4_5.png");
		CAGE_TEST(img->value(50 - 5, 150, 0) < 0.1);
		CAGE_TEST(img->value(50 - 5, 150, 1) > 0.9);
		CAGE_TEST(img->value(50 - 5, 150, 2) > 0.9);
		CAGE_TEST(img->value(50 - 5, 150, 3) > 0.9);
	}

	{
		CAGE_TESTCASE("dilation with nan");
		Holder<Image> img = newImage();
		img->initialize(400, 300, 4, ImageFormatEnum::Float);
		drawCircle(+img);
		for (uint32 y = 0; y < 300; y++)
			for (uint32 x = 0; x < 400; x++)
				if (randomChance() < 0.3)
					img->set(x, y, Vec4::Nan());
		{
			Holder<Timer> timer = newTimer();
			imageDilation(+img, 1, true);
			CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "duration: " + timer->duration());
		}
		imageConvert(+img, ImageFormatEnum::U8);
		img->exportFile("images/dilation/nan.png");
	}
}
