#ifndef guard_noise_h_189F28C1827941F488646FA87D7913F4
#define guard_noise_h_189F28C1827941F488646FA87D7913F4

namespace cage
{
	enum class noiseTypeEnum
	{
		Value,
		Perlin,
		Simplex,
		Cubic,
		Cellular,
		White,
	};

	enum class noiseInterpolationEnum
	{
		Quintic,
		Hermite,
		Linear,
	};

	enum class noiseFractalTypeEnum
	{
		Fbm,
		Billow,
		RigidMulti,
	};

	enum class noiseDistanceEnum
	{
		Euclidean,
		Manhattan, // length in axial directions
		Natural, // blend of euclidean and manhattan
		//EuclideanSquared,
		//Chebychev, // length of the longest axis
	};

	enum class noiseOperationEnum
	{
		None,
		Distance,
		Add,
		Subtract,
		Multiply,
		Divide,
	};

	class CAGE_API noiseClass
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

	struct CAGE_API noiseCreateConfig
	{
		noiseTypeEnum type;
		noiseInterpolationEnum interpolation;
		noiseFractalTypeEnum fractalType;
		noiseDistanceEnum distance;
		noiseOperationEnum operation;
		uint32 seed;
		uint32 octaves;
		real lacunarity;
		real gain;
		real frequency;
		uint8 index0, index1;

		noiseCreateConfig(uint32 seed = 1337);
	};

	CAGE_API holder<noiseClass> newNoise(const noiseCreateConfig &config);
}

#endif // guard_noise_h_189F28C1827941F488646FA87D7913F4
