#include "main.h"
#include <cage-core/marchingCubes.h>
#include <cage-core/polyhedron.h>

namespace
{
	real densityTorus(const vec3 &pos, real major = 75, real minor = 25)
	{
		vec3 c = normalize(pos * vec3(1, 0, 1)) * major;
		return minor - distance(pos, c);
	}

	real density(const vec3 &pos)
	{
		return densityTorus(pos, 5, 2);
	}
}

void testMarchingCubes()
{
	CAGE_TESTCASE("marching cubes");

	MarchingCubesCreateConfig config;
	config.box = aabb(vec3(-5), vec3(15));
#ifdef CAGE_DEBUG
	config.resolutionX = config.resolutionY = config.resolutionZ = 10;
#else
	config.resolutionX = config.resolutionY = config.resolutionZ = 30;
#endif // CAGE_DEBUG
	Holder<MarchingCubes> cubes = newMarchingCubes(config);
	cubes->updateByPosition(Delegate<real(const vec3 &)>().bind<&density>());
	Holder<Polyhedron> poly = cubes->makePolyhedron();
	CAGE_TEST(poly->verticesCount() > 10);
	CAGE_TEST(poly->indicesCount() > 10);
	poly->exportToObjFile({}, "meshes/marchingCubesTorus.obj");
}
