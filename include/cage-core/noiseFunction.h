#ifndef guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4
#define guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4

namespace cage
{
	enum class NoiseTypeEnum : uint32
	{
		Value,
		Perlin,
		Simplex,
		Cubic,
		Cellular,
		White,
	};

	enum class NoiseInterpolationEnum : uint32
	{
		Quintic,
		Hermite,
		Linear,
	};

	enum class NoiseFractalTypeEnum : uint32
	{
		Fbm,
		Billow,
		RigidMulti,
	};

	enum class NoiseDistanceEnum : uint32
	{
		Euclidean,
		Manhattan, // length in axial directions
		Natural, // blend of euclidean and manhattan
		//EuclideanSquared,
		//Chebychev, // length of the longest axis
	};

	enum class NoiseOperationEnum : uint32
	{
		None,
		Distance,
		Add,
		Subtract,
		Multiply,
		Divide,
	};

	class CAGE_API NoiseFunction : private Immovable
	{
	public:
		real evaluate(real position);
		real evaluate(vec2 position);
		real evaluate(vec3 position);
		real evaluate(vec4 position);
		void evaluate(uint32 count, const real positions[], real results[]);
		void evaluate(uint32 count, const vec2 positions[], real results[]);
		void evaluate(uint32 count, const vec3 positions[], real results[]);
		void evaluate(uint32 count, const vec4 positions[], real results[]);
	};

	struct CAGE_API NoiseFunctionCreateConfig
	{
		NoiseTypeEnum type = NoiseTypeEnum::Simplex;
		NoiseInterpolationEnum interpolation = NoiseInterpolationEnum::Quintic;
		NoiseFractalTypeEnum fractalType = NoiseFractalTypeEnum::Fbm;
		NoiseDistanceEnum distance = NoiseDistanceEnum::Euclidean;
		NoiseOperationEnum operation = NoiseOperationEnum::None;
		uint32 seed = 0;
		uint32 octaves = 0;
		real lacunarity = 2;
		real gain = 0.5;
		real frequency = 1;
		uint8 index0 = 0, index1 = 1;

		NoiseFunctionCreateConfig(uint32 seed = 1337);
	};

	CAGE_API Holder<NoiseFunction> newNoiseFunction(const NoiseFunctionCreateConfig &config);
}

#endif // guard_noiseFunction_h_189F28C1827941F488646FA87D7913F4
