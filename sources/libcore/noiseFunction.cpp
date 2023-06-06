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
				case NoiseTypeEnum::Simplex:
					return FastNoise::NoiseType_OpenSimplex2;
				case NoiseTypeEnum::SimplexReduced:
					return FastNoise::NoiseType_OpenSimplex2S;
				case NoiseTypeEnum::Cellular:
					return FastNoise::NoiseType_Cellular;
				case NoiseTypeEnum::Perlin:
					return FastNoise::NoiseType_Perlin;
				case NoiseTypeEnum::Cubic:
					return FastNoise::NoiseType_ValueCubic;
				case NoiseTypeEnum::Value:
					return FastNoise::NoiseType_Value;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid noise type enum");
			}
		}

		FastNoise::FractalType convert(NoiseFractalTypeEnum type)
		{
			switch (type)
			{
				case NoiseFractalTypeEnum::None:
					return FastNoise::FractalType_None;
				case NoiseFractalTypeEnum::Fbm:
					return FastNoise::FractalType_FBm;
				case NoiseFractalTypeEnum::Ridged:
					return FastNoise::FractalType_Ridged;
				case NoiseFractalTypeEnum::PingPong:
					return FastNoise::FractalType_PingPong;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid noise fractal type enum");
			}
		}

		FastNoise::CellularDistanceFunction convert(NoiseDistanceEnum distance)
		{
			switch (distance)
			{
				case NoiseDistanceEnum::Euclidean:
					return FastNoise::CellularDistanceFunction_Euclidean;
				case NoiseDistanceEnum::EuclideanSq:
					return FastNoise::CellularDistanceFunction_EuclideanSq;
				case NoiseDistanceEnum::Manhattan:
					return FastNoise::CellularDistanceFunction_Manhattan;
				case NoiseDistanceEnum::Hybrid:
					return FastNoise::CellularDistanceFunction_Hybrid;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid noise distance enum");
			}
		}

		FastNoise::CellularReturnType convert(NoiseOperationEnum operation)
		{
			switch (operation)
			{
				case NoiseOperationEnum::Cell:
					return FastNoise::CellularReturnType_CellValue;
				case NoiseOperationEnum::Distance:
					return FastNoise::CellularReturnType_Distance;
				case NoiseOperationEnum::Distance2:
					return FastNoise::CellularReturnType_Distance2;
				case NoiseOperationEnum::Add:
					return FastNoise::CellularReturnType_Distance2Add;
				case NoiseOperationEnum::Subtract:
					return FastNoise::CellularReturnType_Distance2Sub;
				case NoiseOperationEnum::Multiply:
					return FastNoise::CellularReturnType_Distance2Mul;
				case NoiseOperationEnum::Divide:
					return FastNoise::CellularReturnType_Distance2Div;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid noise operation enum");
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
		};
	}

	Real NoiseFunction::evaluate(Real position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		return impl->n.GetNoise(position.value, float(0));
	}

	Real NoiseFunction::evaluate(Vec2 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		return impl->n.GetNoise(position[0].value, position[1].value);
	}

	Real NoiseFunction::evaluate(Vec3 position)
	{
		NoiseFunctionImpl *impl = (NoiseFunctionImpl *)this;
		return impl->n.GetNoise(position[0].value, position[1].value, position[2].value);
	}

	void NoiseFunction::evaluate(PointerRange<const Real> x, PointerRange<Real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		const Real *in = x.data();
		for (Real &r : results)
			r = evaluate(*in++);
	}

	void NoiseFunction::evaluate(PointerRange<const Real> x, PointerRange<const Real> y, PointerRange<Real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		CAGE_ASSERT(y.size() == results.size());
		const Real *xx = x.data();
		const Real *yy = y.data();
		for (Real &r : results)
			r = evaluate(Vec2(*xx++, *yy++));
	}

	void NoiseFunction::evaluate(PointerRange<const Real> x, PointerRange<const Real> y, PointerRange<const Real> z, PointerRange<Real> results)
	{
		CAGE_ASSERT(x.size() == results.size());
		CAGE_ASSERT(y.size() == results.size());
		CAGE_ASSERT(z.size() == results.size());
		const Real *xx = x.data();
		const Real *yy = y.data();
		const Real *zz = z.data();
		for (Real &r : results)
			r = evaluate(Vec3(*xx++, *yy++, *zz++));
	}

	void NoiseFunction::evaluate(PointerRange<const Vec2> p, PointerRange<Real> results)
	{
		CAGE_ASSERT(p.size() == results.size());
		const Vec2 *in = p.data();
		for (Real &r : results)
			r = evaluate(*in++);
	}

	void NoiseFunction::evaluate(PointerRange<const Vec3> p, PointerRange<Real> results)
	{
		CAGE_ASSERT(p.size() == results.size());
		const Vec3 *in = p.data();
		for (Real &r : results)
			r = evaluate(*in++);
	}

	NoiseFunctionCreateConfig::NoiseFunctionCreateConfig(uint32 seed) : seed(seed) {}

	Holder<NoiseFunction> newNoiseFunction(const NoiseFunctionCreateConfig &config)
	{
		return systemMemory().createImpl<NoiseFunction, NoiseFunctionImpl>(config);
	}
}
