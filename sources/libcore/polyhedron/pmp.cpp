#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <pmp/SurfaceMesh.h>
#include <pmp/algorithms/SurfaceRemeshing.h>

#include "polyhedron.h"

namespace cage
{
	namespace
	{
		Holder<pmp::SurfaceMesh> toPmp(const Polyhedron *mesh)
		{
			CAGE_ASSERT(mesh->type() == PolyhedronTypeEnum::Triangles);
			Holder<pmp::SurfaceMesh> res = detail::systemArena().createHolder<pmp::SurfaceMesh>();
			{ // vertices
				for (const vec3 &p : mesh->positions())
					res->add_vertex(pmp::Point(p[0].value, p[1].value, p[2].value));
				if (!mesh->normals().empty())
				{
					pmp::VertexProperty<pmp::Point> normals = res->add_vertex_property<pmp::Point>("v:normal");
					for (const auto &p : mesh->normals())
						normals.vector().push_back(pmp::Point(p[0].value, p[1].value, p[2].value));
				}
			}
			if (mesh->indicesCount() > 0)
			{ // indices
				const auto &inds = mesh->indices();
				const uint32 tris = numeric_cast<uint32>(inds.size()) / 3;
				for (uint32 t = 0; t < tris; t++)
					res->add_triangle(pmp::Vertex(inds[t * 3 + 0]), pmp::Vertex(inds[t * 3 + 1]), pmp::Vertex(inds[t * 3 + 2]));
			}
			else
			{
				const uint32 tris = numeric_cast<uint32>(mesh->verticesCount()) / 3;
				for (uint32 t = 0; t < tris; t++)
					res->add_triangle(pmp::Vertex(t * 3 + 0), pmp::Vertex(t * 3 + 1), pmp::Vertex(t * 3 + 2));
			}
			return res;
		}

		void fromPmp(Polyhedron *mesh, const Holder<pmp::SurfaceMesh> &pm)
		{
			CAGE_ASSERT(pm->is_triangle_mesh());
			mesh->clear();
			{ // vertices
				{
					static_assert(sizeof(pmp::Point) == sizeof(vec3), "pmp point size mismatch");
					const auto &vs = pm->positions();
					mesh->positions({ (vec3*)vs.data(), (vec3*)vs.data() + vs.size() });
				}
				if (pm->has_vertex_property("v:normal"))
				{
					const auto &ns = pm->get_vertex_property<pmp::Point>("v:normal").vector();
					mesh->normals({ (vec3*)ns.data(), (vec3*)ns.data() + ns.size() });
				}
			}
			// indices
			for (const auto &f : pm->faces())
			{
				auto t = pm->vertices(f);
				auto v = t.begin();
				uint32 a = (*v).idx(); ++v;
				uint32 b = (*v).idx(); ++v;
				uint32 c = (*v).idx(); ++v;
				CAGE_ASSERT(v == t.end());
				mesh->addTriangle(a, b, c);
			}
		}
	}

	void Polyhedron::simplify(const PolyhedronSimplificationConfig &config)
	{
		Holder<pmp::SurfaceMesh> pm = toPmp(this);
		pmp::SurfaceRemeshing rms(*pm);
		rms.adaptive_remeshing(config.minEdgeLength.value, config.maxEdgeLength.value, config.approximateError.value, config.iterations, config.useProjection);
		fromPmp(this, pm);
	}

	void Polyhedron::regularize(const PolyhedronRegularizationConfig &config)
	{
		Holder<pmp::SurfaceMesh> pm = toPmp(this);
		pmp::SurfaceRemeshing rms(*pm);
		rms.uniform_remeshing(config.targetEdgeLength.value, config.iterations, config.useProjection);
		fromPmp(this, pm);
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
