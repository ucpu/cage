#ifndef guard_signedDistanceFunction_h_sdfgr1esd56r
#define guard_signedDistanceFunction_h_sdfgr1esd56r

#include <cage-core/math.h>

namespace cage
{
	CAGE_CORE_API Real sdfPlane(Vec3 pos, Plane plane);
	CAGE_CORE_API Real sdfSphere(Vec3 pos, Real radius);
	CAGE_CORE_API Real sdfCapsule(Vec3 pos, Real prolong, Real radius);
	CAGE_CORE_API Real sdfCylinder(Vec3 pos, Real halfHeight, Real radius);
	CAGE_CORE_API Real sdfBox(Vec3 pos, Vec3 radius);
	CAGE_CORE_API Real sdfTetrahedron(Vec3 pos, Real radius);
	CAGE_CORE_API Real sdfOctahedron(Vec3 pos, Real radius);
	CAGE_CORE_API Real sdfHexagonalPrism(Vec3 pos, Real halfHeight, Real radius);
	CAGE_CORE_API Real sdfTorus(Vec3 pos, Real major, Real minor);
	CAGE_CORE_API Real sdfKnot(Vec3 pos, Real scale, Real k);
}

#endif // guard_signedDistanceFunction_h_sdfgr1esd56r
