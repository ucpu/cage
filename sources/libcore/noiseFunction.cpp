#include <cage-core/math.h>
#include <cage-core/noiseFunction.h>

#include <FastNoiseLite.h>

#include <vector>

using FastNoise = FastNoiseLite;

namespace cage
{
	namespace
	{
		FastNoise::NoiseType convert(NoiseTypeEnum type)
		{
			switch (type)
			{
			case NoiseTypeEnum::Simplex: return FastNoise::NoiseType_OpenSimplex2;
			case NoiseTypeEnum::SimplexReduced: return FastNoise::NoiseType_OpenSimplex2S;
			case NoiseTypeEnum::Cellular: return FastNoise::NoiseType_Cellular;
			case NoiseTypeEnum::Perlin: return FastNoise::NoiseType_Perlin;
			case NoiseTypeEnum::Cubic: return FastNoise::NoiseType_ValueCubic;
			case NoiseTypeEnum::Value: return FastNoise::NoiseType_Value;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise type enum");
			}
		}

		FastNoise::FractalType convert(NoiseFractalTypeEnum type)
		{
			switch (type)
			{
			case NoiseFractalTypeEnum::None: return FastNoise::FractalType_None;
			case NoiseFractalTypeEnum::Fbm: return FastNoise::FractalType_FBm;
			case NoiseFractalTypeEnum::Ridged: return FastNoise::FractalType_Ridged;
			case NoiseFractalTypeEnum::PingPong: return FastNoise::FractalType_PingPong;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise fractal type enum");
			}
		}

		FastNoise::CellularDistanceFunction convert(NoiseDistanceEnum distance)
		{
			switch (distance)
			{
			case NoiseDistanceEnum::Euclidean: return FastNoise::CellularDistanceFunction_Euclidean;
			case NoiseDistanceEnum::EuclideanSq: return FastNoise::CellularDistanceFunction_EuclideanSq;
			case NoiseDistanceEnum::Manhattan: return FastNoise::CellularDistanceFunction_Manhattan;
			case NoiseDistanceEnum::Hybrid: return FastNoise::CellularDistanceFunction_Hybrid;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise distance enum");
			}
		}

		FastNoise::CellularReturnType convert(NoiseOperationEnum operation)
		{
			switch (operation)
			{
			case NoiseOperationEnum::Cell: return FastNoise::CellularReturnType_CellValue;
			case NoiseOperationEnum::Distance: return FastNoise::CellularReturnType_Distance;
			case NoiseOperationEnum::Distance2: return FastNoise::CellularReturnType_Distance2;
			case NoiseOperationEnum::Add: return FastNoise::CellularReturnType_Distance2Add;
			case NoiseOperationEnum::Subtract: return FastNoise::CellularReturnType_Distance2Sub;
			case NoiseOperationEnum::Multiply: return FastNoise::CellularReturnType_Distance2Mul;
			case NoiseOperationEnum::Divide: return FastNoise::CellularReturnType_Distance2Div;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise operation enum");
			}
		}

		class NoiseFunctionImpl : public NoiseFunction
		{
		public:
			FastNoise n;

			explicit NoiseFunctionImpl(const NoiseFunctionCreateConfig &config) : n(config.seed)
			{
				n.SetFrequency(config.frequency.value);
				n.SetNoiseType(convert(config.type));
				n.SetFractalType(convert(config.fractalType));
				n.SetFractalOctaves(config.octaves);
				n.SetFractalLacunarity(config.lacunarity.value);
				n.SetFractalGain(config.gain.value);
				n.SetCellularDistanceFunction(convert(config.distance));
				n.SetCellularReturnType(convert(config.operation));
			}

			void evaluate(uint32 count, const real x[], real results[])
			{
				for (uint32 i = 0; i < count; i++)
					results[i] = n.GetNoise(x[i].value, float(0));
			}

			void evaluate(uint32 count, const real x[], const real y[], real results[])
			{
				for (uint32 i = 0; i < count; i++)
					results[i] = n.GetNoise(x[i].value, y[i].value);
			}

			void evaluate(uint32 count, const real x[], const real y[], const real z[], real results[])
			{
				for (uint32 i = 0; i < count; i++)
					results[i] = n.GetNoise(x[i].value, y[i].value, z[i].value);
			}

			void evaluate(uint32 count, const real x[], const real y[], const real z[], const real w[], real results[])
			{
				CAGE_THROW_CRITICAL(NotImplemented, "4D noise functions are not implemented");
			}
		};
	}

	real NoiseFunction::evaluate(real position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	real NoiseFunction::evaluate(vec2 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		real result;
		impl->evaluate(1, &position[0], &position[1], &result);
		return result;
	}

	real NoiseFunction::evaluate(vec3 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		real result;
		impl->evaluate(1, &position[0], &position[1], &position[2], &result);
		return result;
	}

	real NoiseFunction::evaluate(vec4 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		real result;
		impl->evaluate(1, &position[0], &position[1], &position[2], &position[3], &result);
		return result;
	}

	void NoiseFunction::evaluate(PointerRange<const real> x, PointerRange<real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		impl->evaluate(numeric_cast<uint32>(x.size()), x.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const real> x, PointerRange<const real> y, PointerRange<real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		CAGE_ASSERT(y.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		impl->evaluate(numeric_cast<uint32>(x.size()), x.data(), y.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const real> x, PointerRange<const real> y, PointerRange<const real> z, PointerRange<real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		CAGE_ASSERT(y.size() == results.size());
		CAGE_ASSERT(z.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		impl->evaluate(numeric_cast<uint32>(x.size()), x.data(), y.data(), z.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const real> x, PointerRange<const real> y, PointerRange<const real> z, PointerRange<const real> w, PointerRange<real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		CAGE_ASSERT(y.size() == results.size());
		CAGE_ASSERT(z.size() == results.size());
		CAGE_ASSERT(w.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		impl->evaluate(numeric_cast<uint32>(x.size()), x.data(), y.data(), z.data(), w.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const vec2> p, PointerRange<real> results)
	{
		CAGE_ASSERT(p.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		std::vector<real> v;
		v.resize(p.size() * 2);
		const uint32 cnt = numeric_cast<uint32>(p.size());
		for (uint32 i = 0; i < cnt; i++)
		{
			v[0 * cnt + i] = p[i][0];
			v[1 * cnt + i] = p[i][1];
		}
		impl->evaluate(cnt, v.data() + 0 * cnt, v.data() + 1 * cnt, results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const vec3> p, PointerRange<real> results)
	{
		CAGE_ASSERT(p.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		std::vector<real> v;
		v.resize(p.size() * 3);
		const uint32 cnt = numeric_cast<uint32>(v.size());
		for (uint32 i = 0; i < cnt; i++)
		{
			v[0 * cnt + i] = p[i][0];
			v[1 * cnt + i] = p[i][1];
			v[2 * cnt + i] = p[i][2];
		}
		impl->evaluate(cnt, v.data() + 0 * cnt, v.data() + 1 * cnt, v.data() + 2 * cnt, results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const vec4> p, PointerRange<real> results)
	{
		CAGE_ASSERT(p.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		std::vector<real> v;
		v.resize(p.size() * 4);
		const uint32 cnt = numeric_cast<uint32>(v.size());
		for (uint32 i = 0; i < cnt; i++)
		{
			v[0 * cnt + i] = p[i][0];
			v[1 * cnt + i] = p[i][1];
			v[2 * cnt + i] = p[i][2];
			v[3 * cnt + i] = p[i][3];
		}
		impl->evaluate(cnt, v.data() + 0 * cnt, v.data() + 1 * cnt, v.data() + 2 * cnt, v.data() + 3 * cnt, results.data());
	}

	NoiseFunctionCreateConfig::NoiseFunctionCreateConfig(uint32 seed) : seed(seed)
	{}

	Holder<NoiseFunction> newNoiseFunction(const NoiseFunctionCreateConfig &config)
	{
		return detail::systemArena().createImpl<NoiseFunction, NoiseFunctionImpl>(config);
	}
}
