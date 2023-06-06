#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include "mesh.h"

#include <cage-core/meshAlgorithms.h>
#include <pmp/SurfaceMesh.h>
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/smoothing.h>

namespace cage
{
	namespace
	{
		Holder<pmp::SurfaceMesh> toPmp(const Mesh *model)
		{
			CAGE_ASSERT(model->type() == MeshTypeEnum::Triangles);
			Holder<pmp::SurfaceMesh> res = systemMemory().createHolder<pmp::SurfaceMesh>();
			{ // vertices
				for (const Vec3 &p : model->positions())
					res->add_vertex(pmp::Point(p[0].value, p[1].value, p[2].value));
				if (!model->normals().empty())
				{
					pmp::VertexProperty<pmp::Normal> normals = res->add_vertex_property<pmp::Normal>("v:normal");
					normals.vector().clear();
					for (const auto &p : model->normals())
						normals.vector().push_back(pmp::Normal(p[0].value, p[1].value, p[2].value));
				}
			}
			if (model->indicesCount() > 0)
			{ // indices
				const auto &inds = model->indices();
				const uint32 tris = numeric_cast<uint32>(inds.size()) / 3;
				for (uint32 t = 0; t < tris; t++)
					res->add_triangle(pmp::Vertex(inds[t * 3 + 0]), pmp::Vertex(inds[t * 3 + 1]), pmp::Vertex(inds[t * 3 + 2]));
			}
			else
			{
				const uint32 tris = numeric_cast<uint32>(model->verticesCount()) / 3;
				for (uint32 t = 0; t < tris; t++)
					res->add_triangle(pmp::Vertex(t * 3 + 0), pmp::Vertex(t * 3 + 1), pmp::Vertex(t * 3 + 2));
			}
			return res;
		}

		void fromPmp(Mesh *model, const Holder<pmp::SurfaceMesh> &pm)
		{
			CAGE_ASSERT(pm->is_triangle_mesh());
			model->clear();
			{ // vertices
				{
					static_assert(sizeof(pmp::Point) == sizeof(Vec3), "pmp point size mismatch");
					const auto &vs = pm->positions();
					model->positions({ (Vec3 *)vs.data(), (Vec3 *)vs.data() + vs.size() });
				}
				if (pm->has_vertex_property("v:normal"))
				{
					static_assert(sizeof(pmp::Normal) == sizeof(Vec3), "pmp normal size mismatch");
					const auto &ns = pm->get_vertex_property<pmp::Normal>("v:normal").vector();
					model->normals({ (Vec3 *)ns.data(), (Vec3 *)ns.data() + ns.size() });
				}
			}
			// indices
			for (const auto &f : pm->faces())
			{
				auto t = pm->vertices(f);
				auto v = t.begin();
				uint32 a = (*v).idx();
				++v;
				uint32 b = (*v).idx();
				++v;
				uint32 c = (*v).idx();
				++v;
				CAGE_ASSERT(v == t.end());
				model->addTriangle(a, b, c);
			}
		}
	}

	void meshSimplify(Mesh *msh, const MeshSimplifyConfig &config)
	{
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh simplification requires triangles mesh");
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh);
			pmp::adaptive_remeshing(*pm, config.minEdgeLength.value, config.maxEdgeLength.value, config.approximateError.value, config.iterations, config.useProjection);
			fromPmp(msh, pm);
		}
		catch (const std::exception &e)
		{
			CAGE_LOG_THROW(e.what());
			CAGE_THROW_ERROR(Exception, "mesh simplification failure");
		}
	}

	void meshRegularize(Mesh *msh, const MeshRegularizeConfig &config)
	{
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh regularization requires triangles mesh");
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh);
			pmp::uniform_remeshing(*pm, config.targetEdgeLength.value, config.iterations, config.useProjection);
			fromPmp(msh, pm);
		}
		catch (const std::exception &e)
		{
			CAGE_LOG_THROW(e.what());
			CAGE_THROW_ERROR(Exception, "mesh regularization failure");
		}
	}

	void meshSmoothing(Mesh *msh, const MeshSmoothingConfig &config)
	{
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh smoothing requires triangles mesh");
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh);
			pmp::explicit_smoothing(*pm, config.iterations, config.uniform);
			fromPmp(msh, pm);
		}
		catch (const std::exception &e)
		{
			CAGE_LOG_THROW(e.what());
			CAGE_THROW_ERROR(Exception, "mesh smoothing failure");
		}
	}
}

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
