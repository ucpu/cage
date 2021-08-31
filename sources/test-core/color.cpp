#include "main.h"
#include <cage-core/color.h>
#include <cage-core/image.h>

void test(const Vec3 &a, const Vec3 &b);

void testColor()
{
	CAGE_TESTCASE("color");

	{
		CAGE_TESTCASE("exact (de)gamma");
		for (uint32 i = 0; i < 20; i++)
		{
			Vec3 a = randomChance3();
			Vec3 b = colorGammaToLinear(a, 2.2);
			CAGE_TEST(b[0] <= a[0]);
			CAGE_TEST(b[1] <= a[1]);
			CAGE_TEST(b[2] <= a[2]);
			Vec3 c = colorLinearToGamma(b);
			test(a, c);
		}
	}

	{
		CAGE_TESTCASE("fast (de)gamma");
		for (uint32 i = 0; i < 20; i++)
		{
			Vec3 a = randomChance3();
			Vec3 b = colorGammaToLinear(a);
			CAGE_TEST(b[0] <= a[0]);
			CAGE_TEST(b[1] <= a[1]);
			CAGE_TEST(b[2] <= a[2]);
			Vec3 c = colorLinearToGamma(b);
			Vec3 d3 = abs(c - a);
			Real d = max(d3[0], max(d3[1], d3[2]));
			CAGE_TEST(255 * d < 3);
		}
	}

	{
		CAGE_TESTCASE("rgb <=> hsv");
		for (uint32 i = 0; i < 20; i++)
		{
			Vec3 a = randomChance3();
			Vec3 b = colorRgbToHsv(a);
			Vec3 c = colorHsvToRgb(b);
			test(a, c);
		}
		test(colorHsvToRgb(Vec3(0, 1, 1)), Vec3(1, 0, 0)); // red
		test(colorHsvToRgb(Vec3(0.3333333, 1, 1)), Vec3(0, 1, 0)); // green
		test(colorHsvToRgb(Vec3(0.6666666, 1, 1)), Vec3(0, 0, 1)); // blue
		test(colorHsvToRgb(Vec3(1, 1, 1)), Vec3(1, 0, 0)); // red again
		test(colorHsvToRgb(Vec3(0, 0, 1)), Vec3(1, 1, 1)); // white
		test(colorHsvToRgb(Vec3(0, 0, 0)), Vec3(0)); // black 1
		test(colorHsvToRgb(Vec3(0, 1, 0)), Vec3(0)); // black 2
		test(colorHsvToRgb(Vec3(0.5, 1, 0)), Vec3(0)); // black 3
		test(colorHsvToRgb(Vec3(0, 0.5, 1)), Vec3(1, 0.5, 0.5));
		test(colorHsvToRgb(Vec3(0, 1, 0.5)), Vec3(0.5, 0, 0));
	}

	{
		CAGE_TESTCASE("rgb <=> hsluv");
		for (uint32 i = 0; i < 20; i++)
		{
			Vec3 a = randomChance3();
			Vec3 b = colorRgbToHsluv(a);
			Vec3 c = colorHsluvToRgb(b);
			test(a, c);
		}
	}

	{
		CAGE_TESTCASE("rgbE");
		for (uint32 i = 0; i < 20; i++)
		{
			Vec3 a = randomChance3() * 0.1 + randomChance() * 0.9;
			uint32 b = colorRgbToRgbe(a);
			Vec3 c = colorRgbeToRgb(b);
			test(a / 50, c / 50); // lower precision intended, because the conversion is lossy
		}
	}

	{
		CAGE_TESTCASE("color interpolation");
		Holder<Image> png = newImage();
		png->initialize(100, 100, 3);
		for (uint32 y = 0; y < 100; y++)
		{
			Vec3 a = colorHsvToRgb(randomChance3());
			Vec3 b = colorHsvToRgb(randomChance3());
			png->set(0, y, a);
			for (uint32 x = 1; x < 99; x++)
				png->set(x, y, interpolateColor(a, b, Real(x) / 99));
			png->set(99, y, b);
		}
		png->exportFile("images/color-interpolation.png");
	}

	{
		CAGE_TESTCASE("color distance");
		CAGE_TEST(distanceColor(Vec3(0), Vec3(1)) > 0.5);
		CAGE_TEST(distanceColor(Vec3(0), Vec3(0.1)) < 0.5);
		CAGE_TEST(distanceColor(Vec3(1, 0, 0), Vec3(1, 0, 0)) < 0.1);
		CAGE_TEST(distanceColor(Vec3(1, 0, 0), Vec3(0, 1, 0)) > 0.3);
	}
}
