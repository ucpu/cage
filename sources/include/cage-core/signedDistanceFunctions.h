#ifndef guard_signedDistanceFunction_h_sdfgr1esd56r
#define guard_signedDistanceFunction_h_sdfgr1esd56r

#include <cage-core/math.h>

namespace cage
{
	CAGE_CORE_API real sdfPlane(const vec3 &pos, const Plane &plane);
	CAGE_CORE_API real sdfSphere(const vec3 &pos, real radius);
	CAGE_CORE_API real sdfCapsule(const vec3 &pos, real prolong, real radius);
	CAGE_CORE_API real sdfCylinder(const vec3 &pos, real halfHeight, real radius);
	CAGE_CORE_API real sdfBox(const vec3 &pos, const vec3 &radius);
	CAGE_CORE_API real sdfTetrahedron(const vec3 &pos, real radius);
	CAGE_CORE_API real sdfOctahedron(const vec3 &pos, real radius);
	CAGE_CORE_API real sdfHexagonalPrism(const vec3 &pos, real halfHeight, real radius);
	CAGE_CORE_API real sdfTorus(const vec3 &pos, real major, real minor);
	CAGE_CORE_API real sdfKnot(const vec3 &pos, real scale, real k);
}

#endif // guard_signedDistanceFunction_h_sdfgr1esd56r
