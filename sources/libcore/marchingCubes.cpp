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
				dens.resize(config.resolutionX * config.resolutionY * config.resolutionZ);
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
		for (uint32 z = 0; z < impl->config.resolutionZ; z++)
		{
			for (uint32 y = 0; y < impl->config.resolutionY; y++)
			{
				for (uint32 x = 0; x < impl->config.resolutionX; x++)
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
		for (uint32 z = 0; z < impl->config.resolutionZ; z++)
		{
			for (uint32 y = 0; y < impl->config.resolutionY; y++)
			{
				for (uint32 x = 0; x < impl->config.resolutionX; x++)
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
		c->importPolyhedron(p.get());
		return c;
	}

	Holder<Polyhedron> MarchingCubes::makePolyhedron() const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;

		dualmc::DualMC<float> mc;
		std::vector<dualmc::Vertex> mcVertices;
		std::vector<dualmc::Quad> mcIndices;
		mc.build((float*)impl->dens.data(), impl->config.resolutionX, impl->config.resolutionY, impl->config.resolutionZ, 0, true, false, mcVertices, mcIndices);

		std::vector<vec3> positions;
		std::vector<vec3> normals;
		std::vector<uint32> indices;
		positions.reserve(mcVertices.size());
		normals.resize(mcVertices.size());
		indices.reserve(mcIndices.size() * 3 / 2);
		const vec3 res = vec3(impl->config.resolutionX, impl->config.resolutionY, impl->config.resolutionZ);
		const vec3 posMult = (impl->config.box.b - impl->config.box.a) / (res - 5);
		const vec3 posAdd = impl->config.box.a - 1.5 * posMult;
		for (const dualmc::Vertex &v : mcVertices)
			positions.push_back(vec3(v.x, v.y, v.z) * posMult + posAdd);
		for (const auto &q : mcIndices)
		{
			const uint32 is[4] = { numeric_cast<uint32>(q.i0), numeric_cast<uint32>(q.i1), numeric_cast<uint32>(q.i2), numeric_cast<uint32>(q.i3) };
			const bool which = distanceSquared(positions[is[0]], positions[is[2]]) < distanceSquared(positions[is[1]], positions[is[3]]); // split the quad by shorter diagonal
			constexpr int first[6] = { 0,1,2, 0,2,3 };
			constexpr int second[6] = { 1,2,3, 1,3,0 };
			for (uint32 i : (which ? first : second))
				indices.push_back(is[i]);
			vec3 n;
			{
				vec3 a = positions[is[0]];
				vec3 b = positions[is[1]];
				vec3 c = positions[is[2]];
				n = normalize(cross(b - a, c - a));
			}
			for (uint32 i : is)
				normals[i] += n;
		}
		for (auto &it : normals)
			it = normalize(it);

		Holder<Polyhedron> result = newPolyhedron();
		result->positions(positions);
		result->normals(normals);
		result->indices(indices);
		if (impl->config.clip)
			polyhedronClip(+result, impl->config.box);
		return result;
	}

	vec3 MarchingCubesCreateConfig::position(uint32 x, uint32 y, uint32 z) const
	{
		CAGE_ASSERT(resolutionX > 5);
		CAGE_ASSERT(resolutionY > 5);
		CAGE_ASSERT(resolutionZ > 5);
		CAGE_ASSERT(x < resolutionX);
		CAGE_ASSERT(y < resolutionY);
		CAGE_ASSERT(z < resolutionZ);
		vec3 f = (vec3(x, y, z) - 2) / (vec3(resolutionX, resolutionY, resolutionZ) - 5);
		return (box.b - box.a) * f + box.a;
	}

	uint32 MarchingCubesCreateConfig::index(uint32 x, uint32 y, uint32 z) const
	{
		CAGE_ASSERT(x < resolutionX);
		CAGE_ASSERT(y < resolutionY);
		CAGE_ASSERT(z < resolutionZ);
		return (z * resolutionY + y) * resolutionX + x;
	}

	Holder<MarchingCubes> newMarchingCubes(const MarchingCubesCreateConfig &config)
	{
		return detail::systemArena().createImpl<MarchingCubes, MarchingCubesImpl>(config);
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
