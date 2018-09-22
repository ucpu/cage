#include "main.h"
#include <cage-core/math.h>
#include <cage-core/color.h>

void test(const vec3 &a, const vec3 &b);

void testColor()
{
	CAGE_TESTCASE("color");
	for (uint32 i = 0; i < 20; i++)
	{
		vec3 a = randomChance3();
		vec3 b = convertRgbToHsv(a);
		vec3 c = convertHsvToRgb(b);
		test(a, c);
	}
	CAGE_LOG(severityEnum::Info, "color conversion", string() + "rgb white -> hsv " + convertRgbToHsv(vec3(1, 1, 1)));
	CAGE_LOG(severityEnum::Info, "color conversion", string() + "rgb black -> hsv " + convertRgbToHsv(vec3(0, 0, 0)));
	CAGE_LOG(severityEnum::Info, "color conversion", string() + "rgb red -> hsv " + convertRgbToHsv(vec3(1, 0, 0)));
	CAGE_LOG(severityEnum::Info, "color conversion", string() + "rgb green -> hsv " + convertRgbToHsv(vec3(0, 1, 0)));
	CAGE_LOG(severityEnum::Info, "color conversion", string() + "rgb blue -> hsv " + convertRgbToHsv(vec3(0, 0, 1)));
	for (uint32 i = 0; i < 20; i++)
	{
		vec3 a = randomChance3() * 0.1 + randomChance() * 0.9;
		uint32 b = convertColorToRgbe(a);
		vec3 c = convertRgbeToColor(b);
		test(a / 50, c / 50); // lower precision intended, because the conversion is lossy
	}
}
