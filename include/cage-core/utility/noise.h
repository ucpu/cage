#ifndef guard_noise_h_189F28C1827941F488646FA87D7913F4
#define guard_noise_h_189F28C1827941F488646FA87D7913F4

namespace cage
{
	enum class noiseDistanceEnum
	{
		Euclidean,
		EuclideanSquared,
		Manhattan, // length in axial directions
		Chebychev, // length of the longest axis
	};

	CAGE_API real noiseValue(uint32 seed,       real  coord);
	CAGE_API real noiseValue(uint32 seed, const vec2 &coord);
	CAGE_API real noiseValue(uint32 seed, const vec3 &coord);
	CAGE_API real noiseValue(uint32 seed, const vec4 &coord);
	CAGE_API real noiseClouds(uint32 seed,       real  coord, uint32 octaves = 3);
	CAGE_API real noiseClouds(uint32 seed, const vec2 &coord, uint32 octaves = 3);
	CAGE_API real noiseClouds(uint32 seed, const vec3 &coord, uint32 octaves = 3);
	CAGE_API real noiseClouds(uint32 seed, const vec4 &coord, uint32 octaves = 3);
	CAGE_API vec4 noiseCell(uint32 seed,       real  coord, noiseDistanceEnum distance = noiseDistanceEnum::Euclidean);
	CAGE_API vec4 noiseCell(uint32 seed, const vec2 &coord, noiseDistanceEnum distance = noiseDistanceEnum::Euclidean);
	CAGE_API vec4 noiseCell(uint32 seed, const vec3 &coord, noiseDistanceEnum distance = noiseDistanceEnum::Euclidean);
	CAGE_API vec4 noiseCell(uint32 seed, const vec4 &coord, noiseDistanceEnum distance = noiseDistanceEnum::Euclidean);
}

#endif // guard_noise_h_189F28C1827941F488646FA87D7913F4
