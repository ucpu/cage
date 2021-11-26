#include "main.h"
#include <cage-core/marchingCubes.h>
#include <cage-core/mesh.h>

void test(Real a, Real b);

namespace
{
	Real sdfSphere(const Vec3 &pos)
	{
		return length(pos - Vec3(5)) - 10;
	}

	Real sdfTiltedPlane(const Vec3 &pos)
	{
		static const Plane pln = Plane(Vec3(), normalize(Vec3(1)));
		Vec3 c = pln.normal * pln.d;
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
			cfg.box.a = Vec3(0);
			cfg.box.b = Vec3(1);
			cfg.resolution = Vec3i(9);
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
			cfg.box.a = Vec3(0);
			cfg.box.b = Vec3(1);
			cfg.resolution = Vec3i(8);
			test(cfg.position(0, 0, 0)[0], Real(-2) / 3);
			test(cfg.position(1, 0, 0)[0], Real(-1) / 3);
			test(cfg.position(2, 0, 0)[0], Real(0) / 3);
			test(cfg.position(3, 0, 0)[0], Real(1) / 3);
			test(cfg.position(4, 0, 0)[0], Real(2) / 3);
			test(cfg.position(5, 0, 0)[0], Real(3) / 3);
			test(cfg.position(6, 0, 0)[0], Real(4) / 3);
			test(cfg.position(7, 0, 0)[0], Real(5) / 3);
		}

		{
			MarchingCubesCreateConfig cfg;
			cfg.box.a = Vec3(-1);
			cfg.box.b = Vec3(1);
			cfg.resolution = Vec3i(7);
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
			cfg.box.a = Vec3(-1);
			cfg.box.b = Vec3(1);
			cfg.resolution = Vec3i(6);
			test(cfg.position(0, 0, 0)[0], Real(-5));
			test(cfg.position(1, 0, 0)[0], Real(-3));
			test(cfg.position(2, 0, 0)[0], Real(-1));
			test(cfg.position(3, 0, 0)[0], Real(1));
			test(cfg.position(4, 0, 0)[0], Real(3));
			test(cfg.position(5, 0, 0)[0], Real(5));
		}
	}

	{
		CAGE_TESTCASE("generate sphere");

		MarchingCubesCreateConfig config;
		config.box = Aabb(Vec3(-10), Vec3(20));
		config.resolution = Vec3i(25);
		Holder<MarchingCubes> cubes = newMarchingCubes(config);
		cubes->updateByPosition(Delegate<Real(const Vec3 &)>().bind<&sdfSphere>());
		Holder<Mesh> poly = cubes->makeMesh();
		CAGE_TEST(poly->verticesCount() > 10);
		CAGE_TEST(poly->indicesCount() > 10);
		poly->exportObjFile("meshes/marchingCubes/sphere.obj");

		{
			CAGE_TESTCASE("coordinates of center of the Sphere");
			Vec3 s;
			for (const Vec3 &p : poly->positions())
				s += p;
			Vec3 c = s / poly->verticesCount();
			CAGE_TEST(distance(c, Vec3(5)) < 0.5);
		}

		{
			CAGE_TESTCASE("sphere radius");
			for (const Vec3 &p : poly->positions())
			{
				Real r = distance(p, Vec3(5));
				CAGE_TEST(r > 9 && r < 11);
			}
		}

		{
			CAGE_TESTCASE("clip x");
			auto p = poly->copy();
			meshClip(+p, Aabb(Vec3(-100, 2, 2), Vec3(100, 8, 8)));
			p->exportObjFile("meshes/marchingCubes/clipX.obj");
		}
		{
			CAGE_TESTCASE("clip y");
			auto p = poly->copy();
			meshClip(+p, Aabb(Vec3(2, -100, 2), Vec3(8, 100, 8)));
			p->exportObjFile("meshes/marchingCubes/clipY.obj");
		}
		{
			CAGE_TESTCASE("clip z");
			auto p = poly->copy();
			meshClip(+p, Aabb(Vec3(2, 2, -100), Vec3(8, 8, 100)));
			p->exportObjFile("meshes/marchingCubes/clipZ.obj");
		}
	}

	{
		CAGE_TESTCASE("generate hexagon");

		MarchingCubesCreateConfig config;
		config.box = Aabb(Vec3(-10), Vec3(10));
		config.resolution = Vec3i(25);
		Holder<MarchingCubes> cubes = newMarchingCubes(config);
		cubes->updateByPosition(Delegate<Real(const Vec3 &)>().bind<&sdfTiltedPlane>());
		Holder<Mesh> poly = cubes->makeMesh();
		poly->exportObjFile("meshes/marchingCubes/hexagon.obj");
	}
}
