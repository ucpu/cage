#include "main.h"

#include <cage-core/geometry.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>

void testColliders()
{
	CAGE_TESTCASE("colliders");

	{
		CAGE_TESTCASE("basic collisions");
		{
			CAGE_TESTCASE("empty colliders");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			CollisionDetectionConfig p(+c1, +c2);
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("no collision");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c2->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->rebuild();
			c2->rebuild();
			CollisionDetectionConfig p(+c1, +c2);
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("one collision");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c2->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0)));
			c1->rebuild();
			c2->rebuild();
			CollisionDetectionConfig p(+c1, +c2);
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.collisionPairs.size() == 1);
			CAGE_TEST(p.collisionPairs[0].a == 0);
			CAGE_TEST(p.collisionPairs[0].b == 0);
		}
		{
			CAGE_TESTCASE("self collision");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c1->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0)));
			c1->rebuild();
			CollisionDetectionConfig p(+c1, +c1);
			CAGE_TEST(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("with transformation (no collision)");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c1->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0)));
			c1->rebuild();
			CollisionDetectionConfig p(+c1, +c1, Transform(), Transform(Vec3(20, 0, 0)));
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("with transformation (one collision)");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c2->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->rebuild();
			c2->rebuild();
			CollisionDetectionConfig p(+c1, +c2, Transform(), Transform(Vec3(), Quat(Degs(), Degs(90), Degs())));
			CAGE_TEST(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("serialization");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c1->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0)));
			Holder<PointerRange<char>> buff = c1->exportBuffer();
			Holder<Collider> c2 = newCollider();
			c2->importBuffer(buff);
			CAGE_TEST(c2->triangles().size() == 3);
			CAGE_TEST(c2->triangles()[2] == c1->triangles()[2]);
		}
		{
			CAGE_TESTCASE("forgot rebuilt");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c1->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0)));
			CollisionDetectionConfig p(+c1, +c1);
			CAGE_TEST_ASSERTED(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("optimize");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0)));
			c1->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1)));
			c1->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0)));
			c1->addTriangle(Triangle(Vec3(-2, 1, -5), Vec3(2, 1, 0), Vec3(0, 1, 5)));
			c1->addTriangle(Triangle(Vec3(-2, 0, 1), Vec3(0, 3, 1), Vec3(2, 0, 1)));
			CAGE_TEST(c1->triangles().size() == 5);
			c1->optimize();
			CAGE_TEST(c1->triangles().size() == 3);
		}
	}

	{
		CAGE_TESTCASE("dynamic collisions");
		Holder<Collider> c1 = newCollider();
		{ // tetrahedron
			const Vec3 a(0, -0.7, 1);
			const Vec3 b(+0.86603, -0.7, -0.5);
			const Vec3 c(-0.86603, -0.7, -0.5);
			const Vec3 d(0, 0.7, 0);
			c1->addTriangle(Triangle(c, b, a));
			c1->addTriangle(Triangle(a, b, d));
			c1->addTriangle(Triangle(b, c, d));
			c1->addTriangle(Triangle(c, a, d));
			c1->rebuild();
		}
		{
			CAGE_TESTCASE("ensure static collision");
			CollisionDetectionConfig p(+c1, +c1, Transform(Vec3(-5, 0, 0), Quat(), 7), Transform(Vec3(5, 0, 0), Quat(), 7));
			CAGE_TEST(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("tetrahedrons meet in the middle");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-5, 0, 0));
			p.bt1 = Transform(Vec3(5, 0, 0));
			p.at2 = Transform();
			p.bt2 = Transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("tetrahedrons do not meet");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-5, 0, 0));
			p.bt1 = Transform(Vec3(5, 0, 0));
			p.at2 = Transform(Vec3(-5, 10, 0));
			p.bt2 = Transform(Vec3(-5, 0, 0));
			CAGE_TEST(!collisionDetection(p));
			CAGE_TEST(!p.fractionBefore.valid());
			CAGE_TEST(!p.fractionContact.valid());
			CAGE_TEST(p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("tetrahedrons are very close");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-1, 0, 0));
			p.bt1 = Transform(Vec3(1, 0, 0));
			p.at2 = Transform();
			p.bt2 = Transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("tetrahedrons are far apart");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-500, 0, 0));
			p.bt1 = Transform(Vec3(500, 0, 0));
			p.at2 = Transform();
			p.bt2 = Transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("one tetrahedron is very large");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-500, 0, 0), Quat(), 50);
			p.bt1 = Transform(Vec3(5, 0, 0));
			p.at2 = Transform(Vec3(), Quat(), 50);
			p.bt2 = Transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("one tetrahedron is very small");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-5, 0, 0), Quat(), 0.01);
			p.bt1 = Transform(Vec3(5, 0, 0));
			p.at2 = Transform(Vec3(), Quat(), 0.01);
			p.bt2 = Transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("no dynamic change (no collision)");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-5, 0, 0));
			p.bt1 = Transform(Vec3(5, 0, 0));
			p.at2 = Transform(Vec3(-5, 0, 0));
			p.bt2 = Transform(Vec3(5, 0, 0));
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("no dynamic change (with collision)");
			CollisionDetectionConfig p(+c1, +c1);
			p.at1 = Transform(Vec3(-0.5001, 0, 0));
			p.bt1 = Transform(Vec3(0.5, 0, 0));
			p.at2 = Transform(Vec3(-0.5, 0, 0));
			p.bt2 = Transform(Vec3(0.5001, 0, 0));
			CAGE_TEST(collisionDetection(p));
		}
	}

	{
		CAGE_TESTCASE("more collisions");
		Holder<Collider> c1 = newCollider();
		{ // tetrahedron 1
			const Vec3 a(0, -0.7, 1);
			const Vec3 b(+0.86603, -0.7, -0.5);
			const Vec3 c(-0.86603, -0.7, -0.5);
			const Vec3 d(0, 0.7, 0);
			const Mat4 off = Mat4(Vec3(10, 0, 0));
			c1->addTriangle(Triangle(c, b, a) * off);
			c1->addTriangle(Triangle(a, b, d) * off);
			c1->addTriangle(Triangle(b, c, d) * off);
			c1->addTriangle(Triangle(c, a, d) * off);
			c1->rebuild();
		}
		Holder<Collider> c2 = newCollider();
		{ // tetrahedron 2
			const Vec3 a(0, -0.7, 1);
			const Vec3 b(+0.86603, -0.7, -0.5);
			const Vec3 c(-0.86603, -0.7, -0.5);
			const Vec3 d(0, 0.7, 0);
			const Mat4 off = Mat4(Vec3(0, 10, 0));
			c2->addTriangle(Triangle(c, b, a) * off);
			c2->addTriangle(Triangle(a, b, d) * off);
			c2->addTriangle(Triangle(b, c, d) * off);
			c2->addTriangle(Triangle(c, a, d) * off);
			c2->rebuild();
		}
		{
			CAGE_TESTCASE("ensure static collision");
			{
				CollisionDetectionConfig p(+c1, +c2);
				CAGE_TEST(!collisionDetection(p));
			}
			{
				CollisionDetectionConfig p(+c1, +c2, Transform(Vec3(-10, 9.5, 0)), Transform());
				CAGE_TEST(collisionDetection(p));
			}
			{
				CollisionDetectionConfig p(+c1, +c2, Transform(), Transform(Vec3(), Quat(Degs(), Degs(), Degs(-90))));
				CAGE_TEST(collisionDetection(p));
			}
		}
		{
			CAGE_TESTCASE("rotations");
			{
				CollisionDetectionConfig p(+c1, +c2);
				p.bt1 = Transform(Vec3(), Quat(Degs(), Degs(), Degs(-100)));
				p.bt2 = Transform(Vec3(), Quat(Degs(), Degs(), Degs(-80)));
				CAGE_TEST(collisionDetection(p));
			}
			{
				CollisionDetectionConfig p(+c1, +c2);
				p.bt1 = Transform(Vec3(0.01, 0, 0), Quat(Degs(), Degs(), Degs(-80)));
				p.bt2 = Transform(Vec3(), Quat(Degs(), Degs(), Degs(-100)));
				CAGE_TEST(collisionDetection(p));
			}
		}
	}

	{
		CAGE_TESTCASE("randomized tests with triangles");

		Holder<Collider> tet = newCollider();
		{
			const Vec3 a(0, -0.7, 1);
			const Vec3 b(+0.86603, -0.7, -0.5);
			const Vec3 c(-0.86603, -0.7, -0.5);
			const Vec3 d(0, 0.7, 0);
			tet->addTriangle(Triangle(c, b, a));
			tet->addTriangle(Triangle(a, b, d));
			tet->addTriangle(Triangle(b, c, d));
			tet->addTriangle(Triangle(c, a, d));
			tet->rebuild();
		}

		uint32 attemptsA = 0, attemptsB = 0;
		while (attemptsA < 5 || attemptsB < 5)
		{
			if (attemptsA + attemptsB > 1000)
			{
				CAGE_LOG_THROW(Stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const Transform t1 = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			const auto &trisTest = [&](const Triangle &l)
			{
				bool res = false;
				for (const Triangle &t : tet->triangles())
					res = res || intersects(l, t * t1);
				return res;
			};

			for (uint32 round = 0; round < 10; round++)
			{
				const Triangle l = Triangle(randomDirection3() * 10, randomDirection3() * 10, randomDirection3() * 10);
				const bool whole = intersects(l, tet.get(), t1);
				const bool individual = trisTest(l);
				CAGE_TEST(whole == individual);
				if (whole)
					attemptsA++;
				else
					attemptsB++;
			}
		}
	}

	{
		CAGE_TESTCASE("randomized tests with lines");

		Holder<Collider> tet = newCollider();
		{
			const Vec3 a(0, -0.7, 1);
			const Vec3 b(+0.86603, -0.7, -0.5);
			const Vec3 c(-0.86603, -0.7, -0.5);
			const Vec3 d(0, 0.7, 0);
			tet->addTriangle(Triangle(c, b, a));
			tet->addTriangle(Triangle(a, b, d));
			tet->addTriangle(Triangle(b, c, d));
			tet->addTriangle(Triangle(c, a, d));
			tet->rebuild();
		}

		uint32 attemptsA = 0, attemptsB = 0;
		while (attemptsA < 5 || attemptsB < 5)
		{
			if (attemptsA + attemptsB > 1000)
			{
				CAGE_LOG_THROW(Stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const Transform t1 = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			const auto &trisTest = [&](const Line &l)
			{
				bool res = false;
				for (const Triangle &t : tet->triangles())
					res = res || intersects(l, t * t1);
				return res;
			};

			for (uint32 round = 0; round < 10; round++)
			{
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				const bool whole = intersects(l, tet.get(), t1);
				const bool individual = trisTest(l);
				CAGE_TEST(whole == individual);
				if (whole)
					attemptsA++;
				else
					attemptsB++;
			}
		}
	}
}
