#include <cstddef> // fix missing size_t in xatlas
#include <xatlas.h>

#include "polyhedron.h"

namespace cage
{
	void Polyhedron::unwrap(const PolyhedronUnwrapConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "unwrap");
	}
}
