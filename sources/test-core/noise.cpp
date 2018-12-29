#include <initializer_list>
#include <vector>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/noise.h>
#include <cage-core/png.h>

namespace
{
#ifdef CAGE_DEBUG
	const sint32 resolution = 256;
#else
	const sint32 resolution = 1024;
#endif // CAGE_DEBUG

	void generateImage(const string &fileName, holder<noiseClass> &noise)
	{
		std::vector<vec2> positions;
		std::vector<real> results;
		positions.reserve(resolution * resolution);
		results.resize(resolution * resolution);
		for (sint32 y = 0; y < resolution; y++)
		{
			for (sint32 x = 0; x < resolution; x++)
				positions.push_back(vec2(x - resolution / 2, y - resolution / 2) * 0.01);
		}
		noise->evaluate(resolution * resolution, positions.data(), results.data());
		holder<pngImageClass> png = newPngImage();
		png->empty(resolution, resolution, 1, 1);
		uint8 *buffer = (uint8*)png->bufferData();
		uint32 index = 0;
		for (sint32 y = 0; y < resolution; y++)
		{
			for (sint32 x = 0; x < resolution; x++)
				*buffer++ = uint8((results[index++] * 127 + 127).value);
		}
		png->encodeFile(fileName);
	}
}

void testNoise()
{
	CAGE_TESTCASE("noise");

	{
		CAGE_TESTCASE("value");
		for (uint32 oct : {1, 2, 3})
		{
			noiseCreateConfig config;
			config.type = noiseTypeEnum::Value;
			config.octaves = oct;
			holder<noiseClass> noise = newNoise(config);
			generateImage(string() + "value_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("perlin");
		for (uint32 oct : {1, 2, 3})
		{
			noiseCreateConfig config;
			config.type = noiseTypeEnum::Perlin;
			config.octaves = oct;
			holder<noiseClass> noise = newNoise(config);
			generateImage(string() + "perlin_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("simplex");
		for (uint32 oct : {1, 2, 3})
		{
			noiseCreateConfig config;
			config.type = noiseTypeEnum::Simplex;
			config.octaves = oct;
			holder<noiseClass> noise = newNoise(config);
			generateImage(string() + "simplex_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("cellular");
		{
			noiseCreateConfig config;
			config.type = noiseTypeEnum::Cellular;
			config.operation = noiseOperationEnum::None;
			holder<noiseClass> noise = newNoise(config);
			generateImage(string() + "cellular_none.png", noise);
		}
		{
			noiseCreateConfig config;
			config.type = noiseTypeEnum::Cellular;
			config.operation = noiseOperationEnum::Distance;
			holder<noiseClass> noise = newNoise(config);
			generateImage(string() + "cellular_distance.png", noise);
		}
		{
			noiseCreateConfig config;
			config.type = noiseTypeEnum::Cellular;
			config.operation = noiseOperationEnum::Divide;
			holder<noiseClass> noise = newNoise(config);
			generateImage(string() + "cellular_divide.png", noise);
		}
	}
}
