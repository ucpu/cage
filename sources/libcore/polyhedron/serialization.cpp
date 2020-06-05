#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include "polyhedron.h"

namespace cage
{
	void Polyhedron::deserialize(const MemoryBuffer &buffer)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "deserialize");
	}

	MemoryBuffer Polyhedron::serialize() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "serialize");
	}
}
