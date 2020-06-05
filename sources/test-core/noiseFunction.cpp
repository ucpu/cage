#include "main.h"
#include <cage-core/math.h>
#include <cage-core/noiseFunction.h>
#include <cage-core/image.h>

#include <initializer_list>
#include <vector>

namespace
{
#ifdef CAGE_DEBUG
	const sint32 resolution = 256;
#else
	const sint32 resolution = 1024;
#endif // CAGE_DEBUG

	void generateImage(const string &fileName, const Holder<NoiseFunction> &noise)
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
		Holder<Image> png = newImage();
		png->initialize(resolution, resolution, 1);
		uint32 index = 0;
		for (sint32 y = 0; y < resolution; y++)
			for (sint32 x = 0; x < resolution; x++)
				png->set(x, y, results[index++]);
		png->exportFile(fileName);
	}
}

void testNoise()
{
	CAGE_TESTCASE("noise");

	{
		CAGE_TESTCASE("value");
		for (uint32 oct : {1, 2, 3})
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Value;
			config.octaves = oct;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(stringizer() + "images/noises/value_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("perlin");
		for (uint32 oct : {1, 2, 3})
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Perlin;
			config.octaves = oct;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(stringizer() + "images/noises/perlin_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("simplex");
		for (uint32 oct : {1, 2, 3})
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Simplex;
			config.octaves = oct;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(stringizer() + "images/noises/simplex_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("cellular");
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::None;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(stringizer() + "images/noises/cellular_none.png", noise);
		}
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::Distance;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(stringizer() + "images/noises/cellular_distance.png", noise);
		}
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::Divide;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(stringizer() + "images/noises/cellular_divide.png", noise);
		}
		{
			NoiseFunctionCreateConfig config2;
			config2.type = NoiseTypeEnum::Value;
			config2.octaves = 2;
			Holder<NoiseFunction> noise2 = newNoiseFunction(config2);
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::NoiseLookup;
			Holder<NoiseFunction> noise = newNoiseFunction(config, templates::move(noise2));
			generateImage(stringizer() + "images/noises/cellular_lookup.png", noise);
		}
	}
}
