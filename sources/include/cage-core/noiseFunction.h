#ifndef guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4
#define guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4

#include "math.h"

namespace cage
{
	enum class NoiseTypeEnum : uint32
	{
		Simplex,
		SimplexReduced,
		Cellular,
		Perlin,
		Cubic,
		Value,
	};

	enum class NoiseFractalTypeEnum : uint32
	{
		None,
		Fbm,
		Ridged,
		PingPong,
	};

	enum class NoiseDistanceEnum : uint32
	{
		Euclidean,
		EuclideanSq,
		Manhattan, // length in axial directions
		Hybrid, // blend of euclidean and manhattan
	};

	enum class NoiseOperationEnum : uint32
	{
		Cell,
		Distance,
		Distance2,
		Add,
		Subtract,
		Multiply,
		Divide,
	};

	class CAGE_CORE_API NoiseFunction : private Immovable
	{
	public:
		Real evaluate(Real position);
		Real evaluate(Vec2 position);
		Real evaluate(Vec3 position);
		Real evaluate(Vec4 position);

		void evaluate(PointerRange<const Real> x, PointerRange<Real> results);
		void evaluate(PointerRange<const Real> x, PointerRange<const Real> y, PointerRange<Real> results);
		void evaluate(PointerRange<const Real> x, PointerRange<const Real> y, PointerRange<const Real> z, PointerRange<Real> results);
		void evaluate(PointerRange<const Real> x, PointerRange<const Real> y, PointerRange<const Real> z, PointerRange<const Real> w, PointerRange<Real> results);

		[[deprecated]] void evaluate(PointerRange<const Vec2> positions, PointerRange<Real> results);
		[[deprecated]] void evaluate(PointerRange<const Vec3> positions, PointerRange<Real> results);
		[[deprecated]] void evaluate(PointerRange<const Vec4> positions, PointerRange<Real> results);
	};

	struct CAGE_CORE_API NoiseFunctionCreateConfig
	{
		NoiseTypeEnum type = NoiseTypeEnum::Simplex;
		NoiseFractalTypeEnum fractalType = NoiseFractalTypeEnum::None;
		NoiseDistanceEnum distance = NoiseDistanceEnum::Euclidean;
		NoiseOperationEnum operation = NoiseOperationEnum::Cell;
		uint32 seed = 0;
		uint32 octaves = 3;
		Real lacunarity = 2;
		Real gain = 0.5;
		Real frequency = 1;

		NoiseFunctionCreateConfig(uint32 seed = 1337);
	};

	CAGE_CORE_API Holder<NoiseFunction> newNoiseFunction(const NoiseFunctionCreateConfig &config);
}

#endif // guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4
