#include <cage-core/math.h>
#include <cage-core/noiseFunction.h>

#include <fastnoise/FastNoise.h>

namespace cage
{
	namespace
	{
		FastNoise::CellularDistanceFunction convert(NoiseDistanceEnum distance)
		{
			switch (distance)
			{
			case NoiseDistanceEnum::Euclidean: return FastNoise::Euclidean;
			case NoiseDistanceEnum::Manhattan: return FastNoise::Manhattan;
			case NoiseDistanceEnum::Natural: return FastNoise::Natural;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise distance enum");
			}
		}

		FastNoise::CellularReturnType convert(NoiseOperationEnum operation)
		{
			switch (operation)
			{
			case NoiseOperationEnum::Add: return FastNoise::Distance2Add;
			case NoiseOperationEnum::Subtract: return FastNoise::Distance2Sub;
			case NoiseOperationEnum::Multiply: return FastNoise::Distance2Mul;
			case NoiseOperationEnum::Divide: return FastNoise::Distance2Div;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise operation enum");
			}
		}

		FastNoise::FractalType convert(NoiseFractalTypeEnum type)
		{
			switch (type)
			{
			case NoiseFractalTypeEnum::Fbm: return FastNoise::FBM;
			case NoiseFractalTypeEnum::Billow: return FastNoise::Billow;
			case NoiseFractalTypeEnum::RigidMulti: return FastNoise::RigidMulti;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise fractal type enum");
			}
		}

		FastNoise::Interp convert(NoiseInterpolationEnum interpolation)
		{
			switch (interpolation)
			{
			case NoiseInterpolationEnum::Quintic: return FastNoise::Quintic;
			case NoiseInterpolationEnum::Hermite: return FastNoise::Hermite;
			case NoiseInterpolationEnum::Linear: return FastNoise::Linear;
			default: CAGE_THROW_CRITICAL(Exception, "invalid noise interpolation enum");
			}
		}

		FastNoise::NoiseType convert(NoiseTypeEnum type, uint32 octaves)
		{
			if (octaves > 1)
			{
				switch (type)
				{
				case NoiseTypeEnum::Value: return FastNoise::ValueFractal;
				case NoiseTypeEnum::Perlin: return FastNoise::PerlinFractal;
				case NoiseTypeEnum::Simplex: return FastNoise::SimplexFractal;
				case NoiseTypeEnum::Cubic: return FastNoise::CubicFractal;
				default: CAGE_THROW_CRITICAL(Exception, "invalid noise type enum (or unsupported number of octaves)");
				}
			}
			else
			{
				switch (type)
				{
				case NoiseTypeEnum::Value: return FastNoise::Value;
				case NoiseTypeEnum::Perlin: return FastNoise::Perlin;
				case NoiseTypeEnum::Simplex: return FastNoise::Simplex;
				case NoiseTypeEnum::Cubic: return FastNoise::Cubic;
				case NoiseTypeEnum::Cellular: return FastNoise::Cellular;
				case NoiseTypeEnum::White: return FastNoise::WhiteNoise;
				default: CAGE_THROW_CRITICAL(Exception, "invalid noise type enum");
				}
			}
		}

		class NoiseFunctionImpl : public NoiseFunction
		{
		public:
			FastNoise n;
			Holder<NoiseFunction> lookupNoise;

			explicit NoiseFunctionImpl(const NoiseFunctionCreateConfig &config, Holder<NoiseFunction> &&lookupNoise) : n(config.seed), lookupNoise(templates::move(lookupNoise))
			{
				n.SetCellularDistanceFunction(convert(config.distance));
				switch (config.operation)
				{
				case NoiseOperationEnum::None:
					n.SetCellularReturnType(FastNoise::CellValue);
					break;
				case NoiseOperationEnum::Distance:
					if (config.index0 == 0)
						n.SetCellularReturnType(FastNoise::Distance);
					else
					{
						n.SetCellularDistance2Indices(0, config.index0);
						n.SetCellularReturnType(FastNoise::Distance2);
					}
					break;
				case NoiseOperationEnum::NoiseLookup:
					n.SetCellularReturnType(FastNoise::NoiseLookup);
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
				if (this->lookupNoise)
					n.SetCellularNoiseLookup(&((NoiseFunctionImpl *)this->lookupNoise.get())->n);
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
		impl->evaluate(1, &position, &result);
		return result;
	}

	real NoiseFunction::evaluate(vec3 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	real NoiseFunction::evaluate(vec4 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		real result;
		impl->evaluate(1, &position, &result);
		return result;
	}

	void NoiseFunction::evaluate(PointerRange<const real> positions, PointerRange<real> results)
	{
		CAGE_ASSERT(positions.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		impl->evaluate(numeric_cast<uint32>(positions.size()), positions.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const vec2> positions, PointerRange<real> results)
	{
		CAGE_ASSERT(positions.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		impl->evaluate(numeric_cast<uint32>(positions.size()), positions.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const vec3> positions, PointerRange<real> results)
	{
		CAGE_ASSERT(positions.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		impl->evaluate(numeric_cast<uint32>(positions.size()), positions.data(), results.data());
	}

	void NoiseFunction::evaluate(PointerRange<const vec4> positions, PointerRange<real> results)
	{
		CAGE_ASSERT(positions.size() == results.size());
		NoiseFunctionImpl *impl = (NoiseFunctionImpl*)this;
		impl->evaluate(numeric_cast<uint32>(positions.size()), positions.data(), results.data());
	}

	NoiseFunctionCreateConfig::NoiseFunctionCreateConfig(uint32 seed) : seed(seed)
	{}

	Holder<NoiseFunction> newNoiseFunction(const NoiseFunctionCreateConfig &config, Holder<NoiseFunction> &&lookupNoise)
	{
		return detail::systemArena().createImpl<NoiseFunction, NoiseFunctionImpl>(config, templates::move(lookupNoise));
	}
}
