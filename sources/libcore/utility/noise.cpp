#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/utility/noise.h>
#include <cage-core/utility/random.h>

#include <utility>

namespace cage
{
	namespace
	{
		uint32 hash(uint32 key)
		{ // integer finalizer hash function
			key ^= key >> 16;
			key *= 0x85ebca6b;
			key ^= key >> 13;
			key *= 0xc2b2ae35;
			key ^= key >> 16;
			return key;
		}

		void decompose(real input, sint32 &whole, real &fraction)
		{
			whole = (sint32)(floor(input).value);
			fraction = input - whole;
			CAGE_ASSERT_RUNTIME(fraction >= 0 && fraction <= 1, input, whole, fraction);
		}

		real hashReal(uint32 key)
		{
			return real(hash(key)) / real((uint32)-1);
		}
	}

	real noiseValue(uint32 seed, real coord)
	{
		sint32 w; real f;
		decompose(coord, w, f);
		w += seed;
		return interpolate(hashReal(w), hashReal(w + 1), smootherstep(f));
	}

	real noiseValue(uint32 seed, const vec2 &coord)
	{
		sint32 w; real f;
		decompose(coord[1], w, f);
		w += seed;
		return interpolate(
			noiseValue(hash(w + 0), coord[0]),
			noiseValue(hash(w + 1), coord[0]),
			smootherstep(f)
		);
	}

	real noiseValue(uint32 seed, const vec3 &coord)
	{
		sint32 w; real f;
		decompose(coord[2], w, f);
		w += seed;
		return interpolate(
			noiseValue(hash(w + 0), vec2(coord)),
			noiseValue(hash(w + 1), vec2(coord)),
			smootherstep(f)
		);
	}

	real noiseValue(uint32 seed, const vec4 &coord)
	{
		sint32 w; real f;
		decompose(coord[3], w, f);
		w += seed;
		return interpolate(
			noiseValue(hash(w + 0), vec3(coord)),
			noiseValue(hash(w + 1), vec3(coord)),
			smootherstep(f)
		);
	}

	namespace
	{
		template<class T>
		real noiseCloudsTemplate(uint32 seed, const T &coord, uint32 octaves)
		{
			real result;
			real scale = 1;
			real weight = 1;
			for (uint32 i = 0; i <= octaves; i++)
			{
				seed = hash(seed);
				result *= 1 - weight;
				result += noiseValue(seed, coord * scale) * weight;
				scale *= 2;
				weight *= 0.5;
			}
			CAGE_ASSERT_RUNTIME(result >= 0 && result <= 1, result, seed, octaves, coord);
			return result;
		}
	}

	real noiseClouds(uint32 seed, real coord, uint32 octaves)
	{
		return noiseCloudsTemplate(seed, coord, octaves);
	}

	real noiseClouds(uint32 seed, const vec2 &coord, uint32 octaves)
	{
		return noiseCloudsTemplate(seed, coord, octaves);
	}

	real noiseClouds(uint32 seed, const vec3 &coord, uint32 octaves)
	{
		return noiseCloudsTemplate(seed, coord, octaves);
	}

	real noiseClouds(uint32 seed, const vec4 &coord, uint32 octaves)
	{
		return noiseCloudsTemplate(seed, coord, octaves);
	}

	namespace
	{
		template<class T, noiseDistanceEnum Distance>
		class metric
		{};

		template<class T>
		class metric<T, noiseDistanceEnum::Euclidean>
		{
		public:
			real operator ()(const T &a, const T &b) const
			{
				return sqrt(metric<T, noiseDistanceEnum::EuclideanSquared>()(a, b));
			}
		};

		template<class T>
		class metric<T, noiseDistanceEnum::EuclideanSquared>
		{
		public:
			real operator ()(const T &a, const T &b) const
			{
				real d = 0;
				for (uint32 n = 0; n < T::Dimension; n++)
					d += sqr(b[n] - a[n]);
				return d;
			}
		};

		template<class T>
		class metric<T, noiseDistanceEnum::Manhattan>
		{
		public:
			real operator ()(const T &a, const T &b) const
			{
				real d = 0;
				for (uint32 n = 0; n < T::Dimension; n++)
					d += abs(b[n] - a[n]);
				return d;
			}
		};

		template<class T>
		class metric<T, noiseDistanceEnum::Chebychev>
		{
		public:
			real operator ()(const T &a, const T &b) const
			{
				real d = 0;
				for (uint32 n = 0; n < T::Dimension; n++)
					d = max(d, abs(b[n] - a[n]));
				return d;
			}
		};

		template<class T, noiseDistanceEnum Distance>
		class sorter
		{
			const T &coord;
			const metric<T, Distance> met;
			real dists[5];

			void insertDistance(real dist)
			{
				dists[0] = dist;
				for (uint32 i = 0; i < 4; i++)
				{
					if (dists[i + 1] > dists[i])
						std::swap(dists[i + 1], dists[i]);
					else
						break;
				}
			}

		public:
			sorter(const T &coord) : coord(coord), met()
			{
				for (uint32 i = 0; i < 5; i++)
					dists[i] = 1;
			}

			void insert(const T &point)
			{
				insertDistance(met(coord, point));
			}

			vec4 result() const
			{
				return vec4(dists[4], dists[3], dists[2], dists[1]);
			}
		};

		template<class T>
		class generator
		{
			randomGenerator rnd;

		public:
			const uint32 points;

			generator(uint32 seed) : rnd(seed, hash(seed)), points(hash(seed) % 3 + 4)
			{}

			T next()
			{
				T res;
				for (uint32 i = 0; i < T::Dimension; i++)
					res[i] = rnd.random();
				return res;
			}
		};

		template<class T, noiseDistanceEnum Distance>
		class cells
		{};

		template<noiseDistanceEnum Distance>
		class cells<real, Distance>
		{
			sorter<real, Distance> srt;

		public:
			cells(uint32 seed, real coord) : srt(coord)
			{
				sint32 whole; real dummy;
				decompose(coord, whole, dummy);
				uint32 sh = hash(seed);
				for (sint32 x = whole - 1; x < whole + 2; x++)
				{
					generator<real> g(sh + hash(x));
					for (uint32 p = 0; p < g.points; p++)
						srt.insert(g.next() + x);
				}
			}

			vec4 operator ()() const
			{
				return srt.result();
			}
		};

		template<noiseDistanceEnum Distance>
		class cells<vec2, Distance>
		{
			sorter<vec2, Distance> srt;

		public:
			cells(uint32 seed, const vec2 &coord) : srt(coord)
			{
				sint32 whole[2]; real dummy;
				for (uint32 i = 0; i < 2; i++)
					decompose(coord[i], whole[i], dummy);
				uint32 sh = hash(seed);
				for (sint32 y = whole[1] - 1; y < whole[1] + 2; y++)
				{
					for (sint32 x = whole[0] - 1; x < whole[0] + 2; x++)
					{
						generator<vec2> g(sh + hash(x) + hash(y));
						for (uint32 p = 0; p < g.points; p++)
							srt.insert(g.next() + vec2(x, y));
					}
				}
			}

			vec4 operator ()() const
			{
				return srt.result();
			}
		};

		template<noiseDistanceEnum Distance>
		class cells<vec3, Distance>
		{
			sorter<vec3, Distance> srt;

		public:
			cells(uint32 seed, const vec3 &coord) : srt(coord)
			{
				sint32 whole[3]; real dummy;
				for (uint32 i = 0; i < 3; i++)
					decompose(coord[i], whole[i], dummy);
				uint32 sh = hash(seed);
				for (sint32 z = whole[2] - 1; z < whole[2] + 2; z++)
				{
					for (sint32 y = whole[1] - 1; y < whole[1] + 2; y++)
					{
						for (sint32 x = whole[0] - 1; x < whole[0] + 2; x++)
						{
							generator<vec3> g(sh + hash(x) + hash(y) + hash(z));
							for (uint32 p = 0; p < g.points; p++)
								srt.insert(g.next() + vec3(x, y, z));
						}
					}
				}
			}

			vec4 operator ()() const
			{
				return srt.result();
			}
		};

		template<noiseDistanceEnum Distance>
		class cells<vec4, Distance>
		{
			sorter<vec4, Distance> srt;

		public:
			cells(uint32 seed, const vec4 &coord) : srt(coord)
			{
				sint32 whole[4]; real dummy;
				for (uint32 i = 0; i < 4; i++)
					decompose(coord[i], whole[i], dummy);
				uint32 sh = hash(seed);
				for (sint32 w = whole[3] - 1; w < whole[3] + 2; w++)
				{
					for (sint32 z = whole[2] - 1; z < whole[2] + 2; z++)
					{
						for (sint32 y = whole[1] - 1; y < whole[1] + 2; y++)
						{
							for (sint32 x = whole[0] - 1; x < whole[0] + 2; x++)
							{
								generator<vec4> g(sh + hash(x) + hash(y) + hash(z) + hash(w));
								for (uint32 p = 0; p < g.points; p++)
									srt.insert(g.next() + vec4(x, y, z, w));
							}
						}
					}
				}
			}

			vec4 operator ()() const
			{
				return srt.result();
			}
		};

		template<class T, noiseDistanceEnum Distance>
		vec4 noiseCellTemplate(uint32 seed, const T &coord)
		{
			cells<T, Distance> c(seed, coord);
			return c();
		}

		template<class T>
		vec4 noiseCellTemplate(uint32 seed, const T &coord, noiseDistanceEnum distance)
		{
			switch (distance)
			{
			case noiseDistanceEnum::Euclidean: return noiseCellTemplate<T, noiseDistanceEnum::Euclidean>(seed, coord);
			case noiseDistanceEnum::EuclideanSquared: return noiseCellTemplate<T, noiseDistanceEnum::EuclideanSquared>(seed, coord);
			case noiseDistanceEnum::Manhattan: return noiseCellTemplate<T, noiseDistanceEnum::Manhattan>(seed, coord);
			case noiseDistanceEnum::Chebychev: return noiseCellTemplate<T, noiseDistanceEnum::Chebychev>(seed, coord);
			}
			CAGE_THROW_CRITICAL(notImplementedException, "unsupported noise distance");
		}
	}

	vec4 noiseCell(uint32 seed, real  coord, noiseDistanceEnum distance)
	{
		return noiseCellTemplate(seed, coord, distance);
	}

	vec4 noiseCell(uint32 seed, const vec2 &coord, noiseDistanceEnum distance)
	{
		return noiseCellTemplate(seed, coord, distance);
	}

	vec4 noiseCell(uint32 seed, const vec3 &coord, noiseDistanceEnum distance)
	{
		return noiseCellTemplate(seed, coord, distance);
	}

	vec4 noiseCell(uint32 seed, const vec4 &coord, noiseDistanceEnum distance)
	{
		return noiseCellTemplate(seed, coord, distance);
	}
}
