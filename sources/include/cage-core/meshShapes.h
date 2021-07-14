#ifndef guard_meshShapes_h_vvcj14du74cu89f
#define guard_meshShapes_h_vvcj14du74cu89f

#include "mesh.h"

namespace cage
{
	CAGE_CORE_API Holder<Mesh> newMeshSphereUv(real radius, uint32 segments, uint32 rings);
	CAGE_CORE_API Holder<Mesh> newMeshSphereRegular(real radius, real edgeLength);
	CAGE_CORE_API Holder<Mesh> newMeshIcosahedron(real radius);
}

#endif // guard_meshShapes_h_vvcj14du74cu89f
