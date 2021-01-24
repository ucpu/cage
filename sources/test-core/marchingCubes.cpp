#include "main.h"
#include <cage-core/marchingCubes.h>
#include <cage-core/polyhedron.h>

void test(real a, real b);

namespace
{
	real sdf(const vec3 &pos)
	{
		return length(pos - vec3(5)) - 10;
	}
}

void testMarchingCubes()
{
	CAGE_TESTCASE("marching cubes");

	{
		CAGE_TESTCASE("coordinates of points on configuration grid");

		// the area is internally larger by one cell on all sides
		// this improves clipping of meshes that intersect the edges
		// clipping is done by the actual size of the provided box

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = vec3(0);
			cfg.box.b = vec3(1);
			cfg.resolution = ivec3(7);
			test(cfg.position(0, 0, 0)[0], -0.25);
			test(cfg.position(1, 0, 0)[0], 0);
			test(cfg.position(2, 0, 0)[0], 0.25);
			test(cfg.position(3, 0, 0)[0], 0.5);
			test(cfg.position(4, 0, 0)[0], 0.75);
			test(cfg.position(5, 0, 0)[0], 1);
			test(cfg.position(6, 0, 0)[0], 1.25);
		}

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = vec3(0);
			cfg.box.b = vec3(1);
			cfg.resolution = ivec3(6);
			test(cfg.position(0, 0, 0)[0], real(-1) / 3);
			test(cfg.position(1, 0, 0)[0], 0);
			test(cfg.position(2, 0, 0)[0], real(1) / 3);
			test(cfg.position(3, 0, 0)[0], real(2) / 3);
			test(cfg.position(4, 0, 0)[0], 1);
			test(cfg.position(5, 0, 0)[0], real(4) / 3);
		}
	}

	{
		CAGE_TESTCASE("generate sphere");

		MarchingCubesCreateConfig config;
		config.box = aabb(vec3(-10), vec3(20));
#ifdef CAGE_DEBUG
		config.resolution = ivec3(20);
#else
		config.resolution = ivec3(30);
#endif // CAGE_DEBUG
		Holder<MarchingCubes> cubes = newMarchingCubes(config);
		cubes->updateByPosition(Delegate<real(const vec3 &)>().bind<&sdf>());
		Holder<Polyhedron> poly = cubes->makePolyhedron();
		CAGE_TEST(poly->verticesCount() > 10);
		CAGE_TEST(poly->indicesCount() > 10);
		poly->exportObjFile({}, "meshes/marchingCubesSphere.obj");

		{
			CAGE_TESTCASE("coordinates of center of the sphere");
			vec3 s;
			for (const vec3 &p : poly->positions())
				s += p;
			vec3 c = s / poly->verticesCount();
			CAGE_TEST(distance(c, vec3(5)) < 0.5);
		}

		{
			CAGE_TESTCASE("sphere radius");
			for (const vec3 &p : poly->positions())
			{
				real r = distance(p, vec3(5));
				CAGE_TEST(r > 9 && r < 11);
			}
		}
	}
}
