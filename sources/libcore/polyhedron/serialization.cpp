#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include "polyhedron.h"

namespace cage
{
	void Polyhedron::load(const MemoryBuffer &buffer)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "load");
	}

	void Polyhedron::load(const void *buffer, uintPtr size)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "load");
	}

	MemoryBuffer Polyhedron::save() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "save");
	}
}
