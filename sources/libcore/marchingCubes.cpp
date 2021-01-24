#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <dualmc.h>

#include <cage-core/marchingCubes.h>
#include <cage-core/polyhedron.h>
#include <cage-core/collider.h>

namespace cage
{
	namespace
	{
		struct MarchingCubesImpl : public MarchingCubes
		{
			const MarchingCubesCreateConfig config;
			std::vector<real> dens;

			MarchingCubesImpl(const MarchingCubesCreateConfig &config) : config(config)
			{
				CAGE_ASSERT(config.resolution[0] > 5);
				CAGE_ASSERT(config.resolution[1] > 5);
				CAGE_ASSERT(config.resolution[2] > 5);
				dens.resize(config.resolution[0] * config.resolution[1] * config.resolution[2]);
			}
		};
	}

	PointerRange<real> MarchingCubes::densities()
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		return impl->dens;
	}

	PointerRange<const real> MarchingCubes::densities() const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;
		return impl->dens;
	}

	void MarchingCubes::densities(const PointerRange<const real> &values)
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		CAGE_ASSERT(impl->dens.size() == values.size());
		detail::memcpy(impl->dens.data(), values.data(), values.size() * sizeof(real));
	}

	real MarchingCubes::density(uint32 x, uint32 y, uint32 z) const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;
		return impl->dens[impl->config.index(x, y, z)];
	}

	void MarchingCubes::density(uint32 x, uint32 y, uint32 z, real value)
	{
		CAGE_ASSERT(value.valid());
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		impl->dens[impl->config.index(x, y, z)] = value;
	}

	void MarchingCubes::updateByCoordinates(const Delegate<real(uint32, uint32, uint32)> &generator)
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		auto it = impl->dens.begin();
		for (uint32 z = 0; z < impl->config.resolution[2]; z++)
		{
			for (uint32 y = 0; y < impl->config.resolution[1]; y++)
			{
				for (uint32 x = 0; x < impl->config.resolution[0]; x++)
				{
					real d = generator(x, y, z);
					CAGE_ASSERT(d.valid());
					*it++ = d;
				}
			}
		}
	}

	void MarchingCubes::updateByPosition(const Delegate<real(const vec3 &)> &generator)
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		auto it = impl->dens.begin();
		for (uint32 z = 0; z < impl->config.resolution[2]; z++)
		{
			for (uint32 y = 0; y < impl->config.resolution[1]; y++)
			{
				for (uint32 x = 0; x < impl->config.resolution[0]; x++)
				{
					real d = generator(impl->config.position(x, y, z));
					CAGE_ASSERT(d.valid());
					*it++ = d;
				}
			}
		}
	}

	Holder<Collider> MarchingCubes::makeCollider() const
	{
		// todo optimized path
		Holder<Polyhedron> p = makePolyhedron();
		Holder<Collider> c = newCollider();
		c->importPolyhedron(+p);
		return c;
	}

	Holder<Polyhedron> MarchingCubes::makePolyhedron() const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;
		const MarchingCubesCreateConfig &cfg = impl->config;

		dualmc::DualMC<float> mc;
		std::vector<dualmc::Vertex> mcVertices;
		std::vector<dualmc::Quad> mcIndices;
		mc.build((float*)impl->dens.data(), cfg.resolution[0], cfg.resolution[1], cfg.resolution[2], 0, true, false, mcVertices, mcIndices);

		std::vector<vec3> positions;
		std::vector<vec3> normals;
		std::vector<uint32> indices;
		positions.reserve(mcVertices.size());
		normals.resize(mcVertices.size());
		indices.reserve(mcIndices.size() * 3 / 2);
		const vec3 res = vec3(cfg.resolution);
		const vec3 posMult = (cfg.box.b - cfg.box.a) / (res - 3);
		const vec3 posAdd = cfg.box.a - posMult;
		for (const dualmc::Vertex &v : mcVertices)
			positions.push_back(vec3(v.x, v.y, v.z) * posMult + posAdd);
		for (const auto &q : mcIndices)
		{
			const uint32 is[4] = { numeric_cast<uint32>(q.i0), numeric_cast<uint32>(q.i1), numeric_cast<uint32>(q.i2), numeric_cast<uint32>(q.i3) };
			const bool which = distanceSquared(positions[is[0]], positions[is[2]]) < distanceSquared(positions[is[1]], positions[is[3]]); // split the quad by shorter diagonal
			constexpr int first[6] = { 0,1,2, 0,2,3 };
			constexpr int second[6] = { 1,2,3, 1,3,0 };
			const int *const selected = (which ? first : second);
			const auto &tri = [&](const int *inds)
			{
				triangle t = triangle(positions[is[inds[0]]], positions[is[inds[1]]], positions[is[inds[2]]]);
				if (!t.degenerated())
				{
					indices.push_back(is[inds[0]]);
					indices.push_back(is[inds[1]]);
					indices.push_back(is[inds[2]]);
					const vec3 n = cross((t[1] - t[0]), (t[2] - t[0])); // no normalization here -> area weighted normals
					normals[is[inds[0]]] += n;
					normals[is[inds[1]]] += n;
					normals[is[inds[2]]] += n;
				}
			};
			tri(selected);
			tri(selected + 3);
		}
		for (vec3 &it : normals)
		{
			if (it != vec3())
				it = normalize(it);
			CAGE_ASSERT(valid(it));
		}

		Holder<Polyhedron> result = newPolyhedron();
		if (indices.empty())
			return result; // if all triangles were degenerated, we would end up with polyhedron with positions and no indices, which is invalid here

		result->positions(positions);
		result->normals(normals);
		result->indices(indices);

		{ // merge vertices
			const vec3 cell = cfg.box.size() / vec3(cfg.resolution);
			const real side = min(cell[0], min(cell[1], cell[2]));
			PolyhedronCloseVerticesMergingConfig cfg;
			cfg.distanceThreshold = side * 0.1;
			polyhedronMergeCloseVertices(+result, cfg);
		}

		if (cfg.clip)
			polyhedronClip(+result, cfg.box);

		return result;
	}

	vec3 MarchingCubesCreateConfig::position(uint32 x, uint32 y, uint32 z) const
	{
		CAGE_ASSERT(resolution[0] > 5);
		CAGE_ASSERT(resolution[1] > 5);
		CAGE_ASSERT(resolution[2] > 5);
		CAGE_ASSERT(x < resolution[0]);
		CAGE_ASSERT(y < resolution[1]);
		CAGE_ASSERT(z < resolution[2]);
		vec3 f = (vec3(x, y, z) - 1) / (vec3(resolution) - 3);
		return (box.b - box.a) * f + box.a;
	}

	uint32 MarchingCubesCreateConfig::index(uint32 x, uint32 y, uint32 z) const
	{
		CAGE_ASSERT(x < resolution[0]);
		CAGE_ASSERT(y < resolution[1]);
		CAGE_ASSERT(z < resolution[2]);
		return (z * resolution[1] + y) * resolution[0] + x;
	}

	Holder<MarchingCubes> newMarchingCubes(const MarchingCubesCreateConfig &config)
	{
		return detail::systemArena().createImpl<MarchingCubes, MarchingCubesImpl>(config);
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
