#ifndef guard_meshShapes_h_vvcj14du74cu89f
#define guard_meshShapes_h_vvcj14du74cu89f

#include <cage-core/mesh.h>

namespace cage
{
	CAGE_CORE_API Holder<Mesh> newMeshSphereUv(Real radius, uint32 segments, uint32 rings);
	CAGE_CORE_API Holder<Mesh> newMeshSphereRegular(Real radius, Real edgeLength);
	CAGE_CORE_API Holder<Mesh> newMeshIcosahedron(Real radius);
}

#endif // guard_meshShapes_h_vvcj14du74cu89f
