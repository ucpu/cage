#include <initializer_list>
#include <vector>

#include "main.h"

#include <cage-core/image.h>
#include <cage-core/math.h>
#include <cage-core/noiseFunction.h>

namespace
{
#ifdef CAGE_DEBUG
	constexpr sint32 resolution = 256;
#else
	constexpr sint32 resolution = 1024;
#endif // CAGE_DEBUG

	void generateImage(const String &fileName, const Holder<NoiseFunction> &noise)
	{
		std::vector<Real> posx, posy, results;
		posx.reserve(resolution * resolution);
		posy.reserve(resolution * resolution);
		results.resize(resolution * resolution);
		for (sint32 y = 0; y < resolution; y++)
		{
			for (sint32 x = 0; x < resolution; x++)
			{
				posx.push_back(Real(x - resolution / 2) * 0.01);
				posy.push_back(Real(y - resolution / 2) * 0.01);
			}
		}
		noise->evaluate(posx, posy, results);
		Real a = Real::Infinity(), b = -Real::Infinity();
		for (const Real v : results)
		{
			a = min(a, v);
			b = max(b, v);
		}
		CAGE_LOG(SeverityEnum::Info, "noise fnc", Stringizer() + "min: " + a + ", max: " + b);
		for (Real &v : results)
			v = (v - a) / (b - a); // normalize into 0 .. 1 range
		Holder<Image> png = newImage();
		png->initialize(resolution, resolution, 1);
		png->colorConfig.gammaSpace = GammaSpaceEnum::Linear;
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
		for (uint32 oct : { 1, 2, 3 })
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Value;
			config.fractalType = NoiseFractalTypeEnum::Fbm;
			config.octaves = oct;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(Stringizer() + "images/noises/value_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("perlin");
		for (uint32 oct : { 1, 2, 3 })
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Perlin;
			config.fractalType = NoiseFractalTypeEnum::Fbm;
			config.octaves = oct;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(Stringizer() + "images/noises/perlin_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("simplex");
		for (uint32 oct : { 1, 2, 3 })
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Simplex;
			config.fractalType = NoiseFractalTypeEnum::Fbm;
			config.octaves = oct;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(Stringizer() + "images/noises/simplex_" + oct + ".png", noise);
		}
	}

	{
		CAGE_TESTCASE("cellular");
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::Cell;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(Stringizer() + "images/noises/cellular_cell.png", noise);
		}
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::Distance;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(Stringizer() + "images/noises/cellular_distance.png", noise);
		}
		{
			NoiseFunctionCreateConfig config;
			config.type = NoiseTypeEnum::Cellular;
			config.operation = NoiseOperationEnum::Divide;
			Holder<NoiseFunction> noise = newNoiseFunction(config);
			generateImage(Stringizer() + "images/noises/cellular_divide.png", noise);
		}
	}

	{
		CAGE_TESTCASE("vectorized");
		Holder<NoiseFunction> noise = newNoiseFunction({});
		{
			CAGE_TESTCASE("1D");
			const Real x[5] = { 1, 2, 3, 4, 5 };
			Real r[5];
			noise->evaluate(x, r);
		}
		{
			CAGE_TESTCASE("2D");
			const Real x[5] = { 1, 2, 3, 4, 5 };
			const Real y[5] = { 2, 3, 4, 5, 6 };
			Real r[5];
			noise->evaluate(x, r);
		}
		{
			CAGE_TESTCASE("3D");
			const Real x[5] = { 1, 2, 3, 4, 5 };
			const Real y[5] = { 2, 3, 4, 5, 6 };
			const Real z[5] = { 3, 4, 5, 6, 7 };
			Real r[5];
			noise->evaluate(x, y, z, r);
		}
		{
			CAGE_TESTCASE("vec2");
			const Vec2 x[5] = { Vec2(1, 2), Vec2(3, 4), Vec2(5, 6), Vec2(7, 8), Vec2(9, 10) };
			Real r[5];
			noise->evaluate(x, r);
		}
		{
			CAGE_TESTCASE("vec3");
			const Vec3 x[5] = { Vec3(1, 2, 3), Vec3(4, 5, 6), Vec3(7, 8, 9), Vec3(10, 11, 12), Vec3(13, 14, 15) };
			Real r[5];
			noise->evaluate(x, r);
		}
	}
}
