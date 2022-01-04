#ifndef guard_signedDistanceFunction_h_sdfgr1esd56r
#define guard_signedDistanceFunction_h_sdfgr1esd56r

#include "math.h"

namespace cage
{
	CAGE_CORE_API Real sdfPlane(const Vec3 &pos, const Plane &plane);
	CAGE_CORE_API Real sdfSphere(const Vec3 &pos, Real radius);
	CAGE_CORE_API Real sdfCapsule(const Vec3 &pos, Real prolong, Real radius);
	CAGE_CORE_API Real sdfCylinder(const Vec3 &pos, Real halfHeight, Real radius);
	CAGE_CORE_API Real sdfBox(const Vec3 &pos, const Vec3 &radius);
	CAGE_CORE_API Real sdfTetrahedron(const Vec3 &pos, Real radius);
	CAGE_CORE_API Real sdfOctahedron(const Vec3 &pos, Real radius);
	CAGE_CORE_API Real sdfHexagonalPrism(const Vec3 &pos, Real halfHeight, Real radius);
	CAGE_CORE_API Real sdfTorus(const Vec3 &pos, Real major, Real minor);
	CAGE_CORE_API Real sdfKnot(const Vec3 &pos, Real scale, Real k);
}

#endif // guard_signedDistanceFunction_h_sdfgr1esd56r
