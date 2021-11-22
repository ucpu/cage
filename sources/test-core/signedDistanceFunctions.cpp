#include "main.h"
#include <cage-core/signedDistanceFunctions.h>
#include <cage-core/marchingCubes.h>
#include <cage-core/mesh.h>
#include <cage-core/macros.h>

namespace
{
	Real tstPlane(const Vec3 &pos) { return sdfPlane(pos, Plane(Vec3(), normalize(Vec3(1)))); }
	Real tstSphere(const Vec3 &pos) { return sdfSphere(pos, 8); }
	Real tstCapsule(const Vec3 &pos) { return sdfCapsule(pos, 4, 6); }
	Real tstCylinder(const Vec3 &pos) { return sdfCylinder(pos, 8, 4); }
	Real tstBox(const Vec3 &pos) { return sdfBox(pos, Vec3(8, 6, 4)); }
	Real tstTetrahedron(const Vec3 &pos) { return sdfTetrahedron(pos, 8); }
	Real tstOctahedron(const Vec3 &pos) { return sdfOctahedron(pos, 8); }
	Real tstHexagonalPrism(const Vec3 &pos) { return sdfHexagonalPrism(pos, 8, 4); }
	Real tstTorus(const Vec3 &pos) { return sdfTorus(pos, 8, 2); }
	Real tstKnot(const Vec3 &pos) { return sdfKnot(pos, 8, 2); }

	using Function = Real(*)(const Vec3 &);

	void sdfTest(Delegate<Real(const Vec3 &)> function, StringLiteral name)
	{
		CAGE_TESTCASE(name);

		MarchingCubesCreateConfig cfg;
		cfg.box = Aabb(Vec3(-10), Vec3(10));
		cfg.clip = true;
#ifdef CAGE_DEBUG
		cfg.resolution = Vec3i(30);
#else
		cfg.resolution = Vec3i(100);
#endif // CAGE_DEBUG
		Holder<MarchingCubes> mc = newMarchingCubes(cfg);
		mc->updateByPosition(function);
		
		Holder<Mesh> msh = mc->makeMesh();
		msh->exportObjFile(Stringizer() + "meshes/sdf/" + name + ".obj");
	}
}

void testSignedDistanceFunctions()
{
	CAGE_TESTCASE("signed distance functions");
#define GCHL_RUN(NAME) sdfTest(Delegate<Real(const Vec3 &)>().bind<&CAGE_JOIN(tst, NAME)>(), CAGE_STRINGIZE(NAME));
	CAGE_EVAL_MEDIUM(CAGE_EXPAND_ARGS(GCHL_RUN, Plane, Sphere, Capsule, Cylinder, Box, Tetrahedron, Octahedron, HexagonalPrism, Torus, Knot));
}
