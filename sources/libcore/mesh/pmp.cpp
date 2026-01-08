#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/smoothing.h>

#include "mesh.h"

#include <cage-core/meshAlgorithms.h>

namespace cage
{
	namespace
	{
		Holder<pmp::SurfaceMesh> toPmp(const Mesh *model, bool ignoreInvalid)
		{
			CAGE_ASSERT(model->type() == MeshTypeEnum::Triangles);
			CAGE_ASSERT(model->indicesCount() > 0);
			Holder<pmp::SurfaceMesh> res = systemMemory().createHolder<pmp::SurfaceMesh>();
			{ // vertices
				res->positions().reserve(model->positions().size());
				for (const Vec3 &p : model->positions())
					res->add_vertex(pmp::Point(p[0].value, p[1].value, p[2].value));
				if (!model->normals().empty())
				{
					pmp::VertexProperty<pmp::Normal> normals = res->add_vertex_property<pmp::Normal>("v:normal");
					normals.vector() = std::vector((pmp::Normal *)model->normals().data(), (pmp::Normal *)model->normals().data() + model->normals().size());
				}
				if (!model->uvs().empty())
				{
					pmp::VertexProperty<pmp::TexCoord> uvs = res->add_vertex_property<pmp::TexCoord>("v:uv");
					uvs.vector() = std::vector((pmp::TexCoord *)model->uvs().data(), (pmp::TexCoord *)model->uvs().data() + model->uvs().size());
				}
			}
			{ // indices
				const auto &inds = model->indices();
				const uint32 tris = numeric_cast<uint32>(inds.size()) / 3;
				if (ignoreInvalid)
				{
					uint32 invalidCount = 0;
					for (uint32 t = 0; t < tris; t++)
					{
						try
						{
							res->add_triangle(pmp::Vertex(inds[t * 3 + 0]), pmp::Vertex(inds[t * 3 + 1]), pmp::Vertex(inds[t * 3 + 2]));
						}
						catch (const pmp::TopologyException &)
						{
							invalidCount++;
						}
					}
					if (invalidCount > 0)
					{
						CAGE_LOG(SeverityEnum::Warning, "pmp", Stringizer() + invalidCount + " triangles were skipped due to complex topology");
					}
				}
				else
				{
					for (uint32 t = 0; t < tris; t++)
						res->add_triangle(pmp::Vertex(inds[t * 3 + 0]), pmp::Vertex(inds[t * 3 + 1]), pmp::Vertex(inds[t * 3 + 2]));
				}
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
				if (pm->has_vertex_property("v:uv"))
				{
					static_assert(sizeof(pmp::TexCoord) == sizeof(Vec2), "pmp texcoord size mismatch");
					const auto &ns = pm->get_vertex_property<pmp::TexCoord>("v:uv").vector();
					model->uvs({ (Vec2 *)ns.data(), (Vec2 *)ns.data() + ns.size() });
				}
			}
			const uint32 vc = model->verticesCount();
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
				CAGE_ASSERT(a < vc && b < vc && c < vc);
				model->addTriangle(a, b, c);
			}
			meshRemoveInvalid(+model);
		}
	}

	void meshSimplify(Mesh *msh, const MeshSimplifyConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh simplification requires triangles mesh");
		meshConvertToIndexed(msh);
		CAGE_ASSERT(!meshDetectInvalid(msh));
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh, config.ignoreInvalid);
			pmp::adaptive_remeshing(*pm, config.minEdgeLength.value, config.maxEdgeLength.value, config.approximateError.value, config.iterations, config.useProjection);
			fromPmp(msh, pm);
		}
		catch (const std::exception &e)
		{
			CAGE_LOG_THROW(e.what());
			CAGE_THROW_ERROR(Exception, "mesh simplification failure");
		}
	}

	void meshDecimate(Mesh *msh, const MeshDecimateConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh decimation requires triangles mesh");
		meshConvertToIndexed(msh);
		CAGE_ASSERT(!meshDetectInvalid(msh));
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh, config.ignoreInvalid);
			pmp::decimate(*pm, config.targetVertices);
			fromPmp(msh, pm);
		}
		catch (const std::exception &e)
		{
			CAGE_LOG_THROW(e.what());
			CAGE_THROW_ERROR(Exception, "mesh decimation failure");
		}
	}

	void meshRegularize(Mesh *msh, const MeshRegularizeConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh regularization requires triangles mesh");
		meshConvertToIndexed(msh);
		CAGE_ASSERT(!meshDetectInvalid(msh));
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh, config.ignoreInvalid);
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
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh smoothing requires triangles mesh");
		meshConvertToIndexed(msh);
		CAGE_ASSERT(!meshDetectInvalid(msh));
		try
		{
			Holder<pmp::SurfaceMesh> pm = toPmp(msh, config.ignoreInvalid);
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
