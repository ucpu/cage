#include <quickhull/QuickHull.hpp>

#include "mesh.h"

#include <cage-core/meshAlgorithms.h>
#include <cage-core/serialization.h>

namespace cage
{
	CAGE_CORE_API Holder<Mesh> meshConvexHull(const Mesh *msh, const MeshConvexHullConfig &config)
	{
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh convex hull requires triangles mesh");

		quickhull::QuickHull<float> qh;
		const auto tmp = qh.getConvexHull(reinterpret_cast<const float *>(msh->positions().data()), msh->positions().size(), false, false);

		Holder<Mesh> res = newMesh();
		{
			static_assert(sizeof(quickhull::Vector3<float>) == sizeof(Vec3));
			PointerRange r(tmp.getVertexBuffer().begin(), tmp.getVertexBuffer().end());
			res->positions(bufferCast<const Vec3>(r));
		}
		{
			std::vector<uint32> inds;
			inds.reserve(tmp.getIndexBuffer().size());
			for (auto it : tmp.getIndexBuffer())
				inds.push_back(numeric_cast<uint32>(it));
			res->indices(inds);
		}
		return res;
	}
}
