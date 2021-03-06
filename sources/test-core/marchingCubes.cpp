#include "main.h"
#include <cage-core/marchingCubes.h>
#include <cage-core/mesh.h>

void test(real a, real b);

namespace
{
	real sdfSphere(const vec3 &pos)
	{
		return length(pos - vec3(5)) - 10;
	}

	real sdfTiltedPlane(const vec3 &pos)
	{
		static const Plane pln = Plane(vec3(), normalize(vec3(1)));
		vec3 c = pln.normal * pln.d;
		return dot(pln.normal, pos - c);
	}
}

void testMarchingCubes()
{
	CAGE_TESTCASE("marching cubes");

	{
		CAGE_TESTCASE("coordinates of points on configuration grid");

		// the area is internally larger by two cells on each side
		// this improves clipping of models that intersect the edges
		// clipping is done by the original box

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = vec3(0);
			cfg.box.b = vec3(1);
			cfg.resolution = ivec3(9);
			test(cfg.position(0, 0, 0)[0], -0.5);
			test(cfg.position(1, 0, 0)[0], -0.25);
			test(cfg.position(2, 0, 0)[0], 0);
			test(cfg.position(3, 0, 0)[0], 0.25);
			test(cfg.position(4, 0, 0)[0], 0.5);
			test(cfg.position(5, 0, 0)[0], 0.75);
			test(cfg.position(6, 0, 0)[0], 1);
			test(cfg.position(7, 0, 0)[0], 1.25);
			test(cfg.position(8, 0, 0)[0], 1.5);
		}

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = vec3(0);
			cfg.box.b = vec3(1);
			cfg.resolution = ivec3(8);
			test(cfg.position(0, 0, 0)[0], real(-2) / 3);
			test(cfg.position(1, 0, 0)[0], real(-1) / 3);
			test(cfg.position(2, 0, 0)[0], real(0) / 3);
			test(cfg.position(3, 0, 0)[0], real(1) / 3);
			test(cfg.position(4, 0, 0)[0], real(2) / 3);
			test(cfg.position(5, 0, 0)[0], real(3) / 3);
			test(cfg.position(6, 0, 0)[0], real(4) / 3);
			test(cfg.position(7, 0, 0)[0], real(5) / 3);
		}

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = vec3(-1);
			cfg.box.b = vec3(1);
			cfg.resolution = ivec3(7);
			test(cfg.position(0, 0, 0)[0], -3);
			test(cfg.position(1, 0, 0)[0], -2);
			test(cfg.position(2, 0, 0)[0], -1);
			test(cfg.position(3, 0, 0)[0], 0);
			test(cfg.position(4, 0, 0)[0], 1);
			test(cfg.position(5, 0, 0)[0], 2);
			test(cfg.position(6, 0, 0)[0], 3);
		}

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = vec3(-1);
			cfg.box.b = vec3(1);
			cfg.resolution = ivec3(6);
			test(cfg.position(0, 0, 0)[0], real(-5));
			test(cfg.position(1, 0, 0)[0], real(-3));
			test(cfg.position(2, 0, 0)[0], real(-1));
			test(cfg.position(3, 0, 0)[0], real(1));
			test(cfg.position(4, 0, 0)[0], real(3));
			test(cfg.position(5, 0, 0)[0], real(5));
		}
	}

	{
		CAGE_TESTCASE("generate sphere");

		MarchingCubesCreateConfig config;
		config.box = Aabb(vec3(-10), vec3(20));
		config.resolution = ivec3(25);
		Holder<MarchingCubes> cubes = newMarchingCubes(config);
		cubes->updateByPosition(Delegate<real(const vec3 &)>().bind<&sdfSphere>());
		Holder<Mesh> poly = cubes->makeMesh();
		CAGE_TEST(poly->verticesCount() > 10);
		CAGE_TEST(poly->indicesCount() > 10);
		poly->exportObjFile({}, "meshes/marchingCubes/sphere.obj");

		{
			CAGE_TESTCASE("coordinates of center of the Sphere");
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

		{
			CAGE_TESTCASE("clip x");
			auto p = poly->copy();
			meshClip(+p, Aabb(vec3(-100, 2, 2), vec3(100, 8, 8)));
			p->exportObjFile({}, "meshes/marchingCubes/clipX.obj");
		}
		{
			CAGE_TESTCASE("clip y");
			auto p = poly->copy();
			meshClip(+p, Aabb(vec3(2, -100, 2), vec3(8, 100, 8)));
			p->exportObjFile({}, "meshes/marchingCubes/clipY.obj");
		}
		{
			CAGE_TESTCASE("clip z");
			auto p = poly->copy();
			meshClip(+p, Aabb(vec3(2, 2, -100), vec3(8, 8, 100)));
			p->exportObjFile({}, "meshes/marchingCubes/clipZ.obj");
		}
	}

	{
		CAGE_TESTCASE("generate hexagon");

		MarchingCubesCreateConfig config;
		config.box = Aabb(vec3(-10), vec3(10));
		config.resolution = ivec3(25);
		Holder<MarchingCubes> cubes = newMarchingCubes(config);
		cubes->updateByPosition(Delegate<real(const vec3 &)>().bind<&sdfTiltedPlane>());
		Holder<Mesh> poly = cubes->makeMesh();
		poly->exportObjFile({}, "meshes/marchingCubes/hexagon.obj");
	}
}
