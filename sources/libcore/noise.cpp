#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/noise.h>

#include <fastnoise/FastNoise.h>

namespace cage
{
	namespace
	{
		FastNoise::CellularDistanceFunction convert(noiseDistanceEnum distance)
		{
			switch (distance)
			{
			case noiseDistanceEnum::Euclidean: return FastNoise::Euclidean;
			case noiseDistanceEnum::Manhattan: return FastNoise::Manhattan;
			case noiseDistanceEnum::Natural: return FastNoise::Natural;
			default: CAGE_THROW_CRITICAL(exception, "invalid noise distance enum");
			}
		}

		FastNoise::CellularReturnType convert(noiseOperationEnum operation)
		{
			switch (operation)
			{
			case noiseOperationEnum::Add: return FastNoise::Distance2Add;
			case noiseOperationEnum::Subtract: return FastNoise::Distance2Sub;
			case noiseOperationEnum::Multiply: return FastNoise::Distance2Mul;
			case noiseOperationEnum::Divide: return FastNoise::Distance2Div;
			default: CAGE_THROW_CRITICAL(exception, "invalid noise operation enum");
			}
		}

		FastNoise::FractalType convert(noiseFractalTypeEnum type)
		{
			switch (type)
			{
			case noiseFractalTypeEnum::Fbm: return FastNoise::FBM;
			case noiseFractalTypeEnum::Billow: return FastNoise::Billow;
			case noiseFractalTypeEnum::RigidMulti: return FastNoise::RigidMulti;
			default: CAGE_THROW_CRITICAL(exception, "invalid noise fractal type enum");
			}
		}

		FastNoise::Interp convert(noiseInterpolationEnum interpolation)
		{
			switch (interpolation)
			{
			case noiseInterpolationEnum::Quintic: return FastNoise::Quintic;
			case noiseInterpolationEnum::Hermite: return FastNoise::Hermite;
			case noiseInterpolationEnum::Linear: return FastNoise::Linear;
			default: CAGE_THROW_CRITICAL(exception, "invalid noise interpolation enum");
			}
		}

		FastNoise::NoiseType convert(noiseTypeEnum type, uint32 octaves)
		{
			if (octaves > 1)
			{
				switch (type)
				{
				case noiseTypeEnum::Value: return FastNoise::ValueFractal;
				case noiseTypeEnum::Perlin: return FastNoise::PerlinFractal;
				case noiseTypeEnum::Simplex: return FastNoise::SimplexFractal;
				case noiseTypeEnum::Cubic: return FastNoise::CubicFractal;
				default: CAGE_THROW_CRITICAL(exception, "invalid noise type enum (or unsupported number of octaves)");
				}
			}
			else
			{
				switch (type)
				{
				case noiseTypeEnum::Value: return FastNoise::Value;
				case noiseTypeEnum::Perlin: return FastNoise::Perlin;
				case noiseTypeEnum::Simplex: return FastNoise::Simplex;
				case noiseTypeEnum::Cubic: return FastNoise::Cubic;
				case noiseTypeEnum::Cellular: return FastNoise::Cellular;
				case noiseTypeEnum::White: return FastNoise::WhiteNoise;
				default: CAGE_THROW_CRITICAL(exception, "invalid noise type enum");
				}
			}
		}

		class noiseImpl : public noiseClass
		{
		public:
			FastNoise n;

			noiseImpl(const noiseCreateConfig &config) : n(config.seed)
			{
				n.SetCellularDistanceFunction(convert(config.distance));
				switch (config.operation)
				{
				case noiseOperationEnum::None:
					n.SetCellularReturnType(FastNoise::CellValue);
					break;
				case noiseOperationEnum::Distance:
					if (config.index0 == 0)
						n.SetCellularReturnType(FastNoise::Distance);
					else
					{
						n.SetCellularDistance2Indices(0, config.index0);
						n.SetCellularReturnType(FastNoise::Distance2);
					}
					break;
				default:
					n.SetCellularDistance2Indices(config.index0, config.index1);
					n.SetCellularReturnType(convert(config.operation));
					break;
				}
				n.SetFractalGain(config.gain.value);
				n.SetFractalLacunarity(config.lacunarity.value);
				n.SetFractalOctaves(config.octaves);
				n.SetFractalType(convert(config.fractalType));
				n.SetFrequency(config.frequency.value);
				n.SetInterp(convert(config.interpolation));
				n.SetNoiseType(convert(config.type, config.octaves));
			}

			void evaluate(uint32 count, const real positions[], real results[])
			{
				for (uint32 i = 0; i < count; i++)
					results[i] = n.GetNoise(positions[i].value, 0);
			}

			void evaluate(uint32 count, const vec2 positions[], real results[])
			{
				for (uint32 i = 0; i < count; i++)
					results[i] = n.GetNoise(positions[i][0].value, positions[i][1].value);
			}

			void evaluate(uint32 count, const vec3 positions[], real results[])
			{
				for (uint32 i = 0; i < count; i++)
					results[i] = n.GetNoise(positions[i][0].value, positions[i][1].value, positions[i][2].value);
			}

			void evaluate(uint32 count, const vec4 positions[], real results[])
			{
				CAGE_THROW_CRITICAL(notImplementedException, "4D noise functions are not implemented");
			}
		};
	}

	real noiseClass::evaluate(real position)
	{
		noiseImpl *impl = (noiseImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	real noiseClass::evaluate(vec2 position)
	{
		noiseImpl *impl = (noiseImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	real noiseClass::evaluate(vec3 position)
	{
		noiseImpl *impl = (noiseImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	real noiseClass::evaluate(vec4 position)
	{
		noiseImpl *impl = (noiseImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	void noiseClass::evaluate(uint32 count, const real positions[], real results[])
	{
		noiseImpl *impl = (noiseImpl*)this;
		impl->evaluate(count, positions, results);
	}

	void noiseClass::evaluate(uint32 count, const vec2 positions[], real results[])
	{
		noiseImpl *impl = (noiseImpl*)this;
		impl->evaluate(count, positions, results);
	}

	void noiseClass::evaluate(uint32 count, const vec3 positions[], real results[])
	{
		noiseImpl *impl = (noiseImpl*)this;
		impl->evaluate(count, positions, results);
	}

	void noiseClass::evaluate(uint32 count, const vec4 positions[], real results[])
	{
		noiseImpl *impl = (noiseImpl*)this;
		impl->evaluate(count, positions, results);
	}

	noiseCreateConfig::noiseCreateConfig(uint32 seed) : seed(seed), type(noiseTypeEnum::Simplex), interpolation(noiseInterpolationEnum::Quintic), fractalType(noiseFractalTypeEnum::Fbm), distance(noiseDistanceEnum::Euclidean), operation(noiseOperationEnum::None), octaves(0), lacunarity(2), gain(0.5), frequency(1), index0(0), index1(1)
	{}

	holder<noiseClass> newNoise(const noiseCreateConfig &config)
	{
		return detail::systemArena().createImpl<noiseClass, noiseImpl>(config);
	}
}
