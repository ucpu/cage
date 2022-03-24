#include <cage-core/marchingCubes.h>
#include <cage-core/mesh.h>
#include <cage-core/collider.h>

#include <robin_hood.h>
#include <dualmc.h>

#include <vector>

namespace cage
{
	namespace
	{
		struct MarchingCubesImpl : public MarchingCubes
		{
			const MarchingCubesCreateConfig config;
			std::vector<Real> dens;

			MarchingCubesImpl(const MarchingCubesCreateConfig &config) : config(config)
			{
				CAGE_ASSERT(config.resolution[0] > 5);
				CAGE_ASSERT(config.resolution[1] > 5);
				CAGE_ASSERT(config.resolution[2] > 5);
				dens.resize(config.resolution[0] * config.resolution[1] * config.resolution[2]);
			}
		};

		void removeNonManifoldTriangles(Mesh *poly)
		{
			CAGE_ASSERT(poly->type() == MeshTypeEnum::Triangles);
			CAGE_ASSERT(poly->indicesCount() > 0);
			CAGE_ASSERT((poly->indicesCount() % 3) == 0);

			struct EdgeFace
			{
				uint32 e1 = m, e2 = m;
				uint32 f = 0;
			};

			struct EdgeHash
			{
				std::size_t operator () (const EdgeFace &l) const
				{
					std::hash<uint32> h;
					return h(l.e1) ^ h(l.e1 ^ l.e2 + 5641851);
				}
			};

			struct EdgeEqual
			{
				bool operator () (const EdgeFace &l, const EdgeFace &r) const
				{
					return std::make_pair(l.e1, l.e2) == std::make_pair(r.e1, r.e2);
				}
			};

			robin_hood::unordered_set<EdgeFace, EdgeHash, EdgeEqual> edges;
			robin_hood::unordered_set<uint32> singularFaces;
			const uint32 cnt = poly->indicesCount();
			for (uint32 i = 0; i < cnt; i += 3)
			{
				const uint32 a = poly->indices()[i + 0];
				const uint32 b = poly->indices()[i + 1];
				const uint32 c = poly->indices()[i + 2];
				for (const EdgeFace &p : { EdgeFace{a, b, i}, EdgeFace{b, c, i}, EdgeFace{c, a, i} })
				{
					auto it = edges.find(p);
					if (it == edges.end())
						edges.insert(p);
					else
					{
						singularFaces.insert(p.f);
						singularFaces.insert(it->f);
					}
				}
			}

			if (singularFaces.empty())
				return;

			std::vector<uint32> inds;
			inds.reserve(cnt);
			for (uint32 i = 0; i < cnt; i += 3)
			{
				if (singularFaces.count(i))
					continue;
				inds.push_back(poly->indices()[i + 0]);
				inds.push_back(poly->indices()[i + 1]);
				inds.push_back(poly->indices()[i + 2]);
			}
			poly->indices(inds);

			// todo detect singular vertices

			// todo hole-filling for removed faces

			if (poly->indicesCount() == 0)
				poly->clear(); // if all triangles were removed, we would end up with mesh with positions and no indices, which is invalid here
		}
	}

	PointerRange<Real> MarchingCubes::densities()
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		return impl->dens;
	}

	PointerRange<const Real> MarchingCubes::densities() const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;
		return impl->dens;
	}

	void MarchingCubes::densities(const PointerRange<const Real> &values)
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		CAGE_ASSERT(impl->dens.size() == values.size());
		detail::memcpy(impl->dens.data(), values.data(), values.size() * sizeof(Real));
	}

	Real MarchingCubes::density(uint32 x, uint32 y, uint32 z) const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;
		return impl->dens[impl->config.index(x, y, z)];
	}

	void MarchingCubes::density(uint32 x, uint32 y, uint32 z, Real value)
	{
		CAGE_ASSERT(value.valid());
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		impl->dens[impl->config.index(x, y, z)] = value;
	}

	void MarchingCubes::updateByCoordinates(const Delegate<Real(uint32, uint32, uint32)> &generator)
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		auto it = impl->dens.begin();
		for (uint32 z = 0; z < numeric_cast<uint32>(impl->config.resolution[2]); z++)
		{
			for (uint32 y = 0; y < numeric_cast<uint32>(impl->config.resolution[1]); y++)
			{
				for (uint32 x = 0; x < numeric_cast<uint32>(impl->config.resolution[0]); x++)
				{
					Real d = generator(x, y, z);
					CAGE_ASSERT(d.valid());
					*it++ = d;
				}
			}
		}
	}

	void MarchingCubes::updateByPosition(const Delegate<Real(const Vec3 &)> &generator)
	{
		MarchingCubesImpl *impl = (MarchingCubesImpl*)this;
		auto it = impl->dens.begin();
		for (uint32 z = 0; z < numeric_cast<uint32>(impl->config.resolution[2]); z++)
		{
			for (uint32 y = 0; y < numeric_cast<uint32>(impl->config.resolution[1]); y++)
			{
				for (uint32 x = 0; x < numeric_cast<uint32>(impl->config.resolution[0]); x++)
				{
					Real d = generator(impl->config.position(x, y, z));
					CAGE_ASSERT(d.valid());
					*it++ = d;
				}
			}
		}
	}

	Holder<Collider> MarchingCubes::makeCollider() const
	{
		// todo optimized path
		Holder<Mesh> p = makeMesh();
		Holder<Collider> c = newCollider();
		c->importMesh(+p);
		return c;
	}

	Holder<Mesh> MarchingCubes::makeMesh() const
	{
		const MarchingCubesImpl *impl = (const MarchingCubesImpl*)this;
		const MarchingCubesCreateConfig &cfg = impl->config;

		dualmc::DualMC<float> mc;
		std::vector<dualmc::Vertex> mcVertices;
		std::vector<dualmc::Quad> mcIndices;
		mc.build((float*)impl->dens.data(), cfg.resolution[0], cfg.resolution[1], cfg.resolution[2], 0, true, false, mcVertices, mcIndices);

		std::vector<Vec3> positions;
		std::vector<Vec3> normals;
		std::vector<uint32> indices;
		positions.reserve(mcVertices.size());
		normals.resize(mcVertices.size());
		indices.reserve(mcIndices.size() * 3 / 2);
		{
			const Vec3 posAdd = cfg.position(0, 0, 0);
			const Vec3 posMult = cfg.box.size() / (Vec3(cfg.resolution) - 5);
			for (const dualmc::Vertex &v : mcVertices)
				positions.push_back(Vec3(v.x, v.y, v.z) * posMult + posAdd);
		}
		for (const auto &q : mcIndices)
		{
			const uint32 is[4] = { numeric_cast<uint32>(q.i0), numeric_cast<uint32>(q.i1), numeric_cast<uint32>(q.i2), numeric_cast<uint32>(q.i3) };
			const bool which = distanceSquared(positions[is[0]], positions[is[2]]) < distanceSquared(positions[is[1]], positions[is[3]]); // split the quad by shorter diagonal
			static constexpr int first[6] = { 0,1,2, 0,2,3 };
			static constexpr int second[6] = { 1,2,3, 1,3,0 };
			const int *const selected = (which ? first : second);
			const auto &tri = [&](const int *inds)
			{
				Triangle t = Triangle(positions[is[inds[0]]], positions[is[inds[1]]], positions[is[inds[2]]]);
				if (!t.degenerated())
				{
					indices.push_back(is[inds[0]]);
					indices.push_back(is[inds[1]]);
					indices.push_back(is[inds[2]]);
					const Vec3 n = cross((t[1] - t[0]), (t[2] - t[0])); // no normalization here -> area weighted normals
					normals[is[inds[0]]] += n;
					normals[is[inds[1]]] += n;
					normals[is[inds[2]]] += n;
				}
			};
			tri(selected);
			tri(selected + 3);
		}
		for (Vec3 &it : normals)
		{
			if (it != Vec3())
				it = normalize(it);
			CAGE_ASSERT(valid(it));
		}

		Holder<Mesh> result = newMesh();
		if (indices.empty())
			return result; // if all triangles were degenerated, we would end up with mesh with positions and no indices, which is invalid here

		result->positions(positions);
		result->normals(normals);
		result->indices(indices);

		removeNonManifoldTriangles(+result);

		if (cfg.clip)
			meshClip(+result, cfg.box);
		else
			meshMergeCloseVertices(+result, {});

		return result;
	}

	Vec3 MarchingCubesCreateConfig::position(uint32 x, uint32 y, uint32 z) const
	{
		CAGE_ASSERT(resolution[0] > 5);
		CAGE_ASSERT(resolution[1] > 5);
		CAGE_ASSERT(resolution[2] > 5);
		CAGE_ASSERT(x < numeric_cast<uint32>(resolution[0]));
		CAGE_ASSERT(y < numeric_cast<uint32>(resolution[1]));
		CAGE_ASSERT(z < numeric_cast<uint32>(resolution[2]));
		Vec3 f = (Vec3(x, y, z) - 2) / (Vec3(resolution) - 5);
		return (box.b - box.a) * f + box.a;
	}

	uint32 MarchingCubesCreateConfig::index(uint32 x, uint32 y, uint32 z) const
	{
		CAGE_ASSERT(x < numeric_cast<uint32>(resolution[0]));
		CAGE_ASSERT(y < numeric_cast<uint32>(resolution[1]));
		CAGE_ASSERT(z < numeric_cast<uint32>(resolution[2]));
		return (z * resolution[1] + y) * resolution[0] + x;
	}

	Holder<MarchingCubes> newMarchingCubes(const MarchingCubesCreateConfig &config)
	{
		return systemMemory().createImpl<MarchingCubes, MarchingCubesImpl>(config);
	}
}
