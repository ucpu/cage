#include "main.h"
#include <cage-core/signedDistanceFunctions.h>
#include <cage-core/marchingCubes.h>
#include <cage-core/mesh.h>
#include <cage-core/macros.h>

namespace
{
	real tstPlane(const vec3 &pos) { return sdfPlane(pos, Plane(vec3(), normalize(vec3(1)))); }
	real tstSphere(const vec3 &pos) { return sdfSphere(pos, 8); }
	real tstCapsule(const vec3 &pos) { return sdfCapsule(pos, 4, 6); }
	real tstCylinder(const vec3 &pos) { return sdfCylinder(pos, 8, 4); }
	real tstBox(const vec3 &pos) { return sdfBox(pos, vec3(8, 6, 4)); }
	real tstTetrahedron(const vec3 &pos) { return sdfTetrahedron(pos, 8); }
	real tstOctahedron(const vec3 &pos) { return sdfOctahedron(pos, 8); }
	real tstHexagonalPrism(const vec3 &pos) { return sdfHexagonalPrism(pos, 8, 4); }
	real tstTorus(const vec3 &pos) { return sdfTorus(pos, 8, 2); }
	real tstKnot(const vec3 &pos) { return sdfKnot(pos, 8, 2); }

	using Function = real(*)(const vec3 &);

	void sdfTest(Delegate<real(const vec3 &)> function, StringLiteral name)
	{
		CAGE_TESTCASE(name);

		MarchingCubesCreateConfig cfg;
		cfg.box = Aabb(vec3(-10), vec3(10));
		cfg.clip = true;
#ifdef CAGE_DEBUG
		cfg.resolution = ivec3(30);
#else
		cfg.resolution = ivec3(100);
#endif // CAGE_DEBUG
		Holder<MarchingCubes> mc = newMarchingCubes(cfg);
		mc->updateByPosition(function);
		
		Holder<Mesh> msh = mc->makeMesh();
		msh->exportObjFile({}, stringizer() + "meshes/sdf/" + name + ".obj");
	}
}

void testSignedDistanceFunctions()
{
	CAGE_TESTCASE("signed distance functions");
#define GCHL_RUN(NAME) sdfTest(Delegate<real(const vec3 &)>().bind<&CAGE_JOIN(tst, NAME)>(), CAGE_STRINGIZE(NAME));
	CAGE_EVAL_MEDIUM(CAGE_EXPAND_ARGS(GCHL_RUN, Plane, Sphere, Capsule, Cylinder, Box, Tetrahedron, Octahedron, HexagonalPrism, Torus, Knot));
}
