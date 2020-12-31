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
		real evaluate(real position);
		real evaluate(vec2 position);
		real evaluate(vec3 position);
		real evaluate(vec4 position);

		void evaluate(PointerRange<const real> x, PointerRange<real> results);
		void evaluate(PointerRange<const real> x, PointerRange<const real> y, PointerRange<real> results);
		void evaluate(PointerRange<const real> x, PointerRange<const real> y, PointerRange<const real> z, PointerRange<real> results);
		void evaluate(PointerRange<const real> x, PointerRange<const real> y, PointerRange<const real> z, PointerRange<const real> w, PointerRange<real> results);

		[[deprecated]] void evaluate(PointerRange<const vec2> positions, PointerRange<real> results);
		[[deprecated]] void evaluate(PointerRange<const vec3> positions, PointerRange<real> results);
		[[deprecated]] void evaluate(PointerRange<const vec4> positions, PointerRange<real> results);
	};

	struct CAGE_CORE_API NoiseFunctionCreateConfig
	{
		NoiseTypeEnum type = NoiseTypeEnum::Simplex;
		NoiseFractalTypeEnum fractalType = NoiseFractalTypeEnum::None;
		NoiseDistanceEnum distance = NoiseDistanceEnum::Euclidean;
		NoiseOperationEnum operation = NoiseOperationEnum::Cell;
		uint32 seed = 0;
		uint32 octaves = 3;
		real lacunarity = 2;
		real gain = 0.5;
		real frequency = 1;

		NoiseFunctionCreateConfig(uint32 seed = 1337);
	};

	CAGE_CORE_API Holder<NoiseFunction> newNoiseFunction(const NoiseFunctionCreateConfig &config);
}

#endif // guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4
