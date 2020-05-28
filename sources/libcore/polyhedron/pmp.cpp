#ifdef _MSC_VER
#pragma warning(push, 0)
#include <pmp/SurfaceMesh.h>
#pragma warning(pop)
#else
#include <pmp/SurfaceMesh.h>
#endif

#include "polyhedron.h"

namespace cage
{
	namespace cage
	{

	}

	void Polyhedron::simplify(const PolyhedronSimplificationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "simplify");
	}

	void Polyhedron::regularize(const PolyhedronRegularizationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "regularize");
	}
}
