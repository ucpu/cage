#include <initializer_list>
#include <vector>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/noiseFunction.h>
#include <cage-core/image.h>

namespace
{
#ifdef CAGE_DEBUG
	const sint32 resolution = 256;
#else
	const sint32 resolution = 1024;
#endif // CAGE_DEBUG

	void generateImage(const string &fileName, holder<noiseFunction> &noise)
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
		holder<image> png = newImage();
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
			noiseFunctionCreateConfig config;
			config.type = noiseTypeEnum::Value;
			config.octaves = oct;
			holder<noiseFunction> noise = newNoiseFunction(config);
			generateImage(string() + "images/value_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("perlin");
		for (uint32 oct : {1, 2, 3})
		{
			noiseFunctionCreateConfig config;
			config.type = noiseTypeEnum::Perlin;
			config.octaves = oct;
			holder<noiseFunction> noise = newNoiseFunction(config);
			generateImage(string() + "images/perlin_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("simplex");
		for (uint32 oct : {1, 2, 3})
		{
			noiseFunctionCreateConfig config;
			config.type = noiseTypeEnum::Simplex;
			config.octaves = oct;
			holder<noiseFunction> noise = newNoiseFunction(config);
			generateImage(string() + "images/simplex_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("cellular");
		{
			noiseFunctionCreateConfig config;
			config.type = noiseTypeEnum::Cellular;
			config.operation = noiseOperationEnum::None;
			holder<noiseFunction> noise = newNoiseFunction(config);
			generateImage(string() + "images/cellular_none.png", noise);
		}
		{
			noiseFunctionCreateConfig config;
			config.type = noiseTypeEnum::Cellular;
			config.operation = noiseOperationEnum::Distance;
			holder<noiseFunction> noise = newNoiseFunction(config);
			generateImage(string() + "images/cellular_distance.png", noise);
		}
		{
			noiseFunctionCreateConfig config;
			config.type = noiseTypeEnum::Cellular;
			config.operation = noiseOperationEnum::Divide;
			holder<noiseFunction> noise = newNoiseFunction(config);
			generateImage(string() + "images/cellular_divide.png", noise);
		}
	}
}
