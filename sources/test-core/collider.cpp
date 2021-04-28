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
			CollisionDetectionParams p(c1.get(), c2.get());
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("no collision");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(Triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->rebuild();
			c2->rebuild();
			CollisionDetectionParams p(c1.get(), c2.get());
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("one collision");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(Triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			c2->rebuild();
			CollisionDetectionParams p(c1.get(), c2.get());
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.collisionPairs.size() == 1);
			CAGE_TEST(p.collisionPairs[0].a == 0);
			CAGE_TEST(p.collisionPairs[0].b == 0);
		}
		{
			CAGE_TESTCASE("self collision");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(Triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(Triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CollisionDetectionParams p(c1.get(), c1.get());
			CAGE_TEST(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("with transformation (no collision)");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(Triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(Triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CollisionDetectionParams p(c1.get(), c1.get(), transform(), transform(vec3(20, 0, 0)));
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("with transformation (one collision)");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(Triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->rebuild();
			c2->rebuild();
			CollisionDetectionParams p(c1.get(), c2.get(), transform(), transform(vec3(), quat(degs(), degs(90), degs())));
			CAGE_TEST(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("serialization");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(Triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(Triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			Holder<PointerRange<char>> buff = c1->serialize();
			Holder<Collider> c2 = newCollider();
			c2->deserialize(buff);
			CAGE_TEST(c2->triangles().size() == 3);
			CAGE_TEST(c2->triangles()[2] == c1->triangles()[2]);
		}
		{
			CAGE_TESTCASE("forgot rebuilt");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(Triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(Triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(Triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			CollisionDetectionParams p(c1.get(), c1.get());
			CAGE_TEST_ASSERTED(collisionDetection(p));
		}
	}

	{
		CAGE_TESTCASE("dynamic collisions");
		Holder<Collider> c1 = newCollider();
		{ // tetrahedron
			const vec3 a(0, -0.7, 1);
			const vec3 b(+0.86603, -0.7, -0.5);
			const vec3 c(-0.86603, -0.7, -0.5);
			const vec3 d(0, 0.7, 0);
			c1->addTriangle(Triangle(c, b, a));
			c1->addTriangle(Triangle(a, b, d));
			c1->addTriangle(Triangle(b, c, d));
			c1->addTriangle(Triangle(c, a, d));
			c1->rebuild();
		}
		{
			CAGE_TESTCASE("ensure static collision");
			CollisionDetectionParams p(c1.get(), c1.get(), transform(vec3(-5, 0, 0), quat(), 7), transform(vec3(5, 0, 0), quat(), 7));
			CAGE_TEST(collisionDetection(p));
		}
		{
			CAGE_TESTCASE("tetrahedrons meet in the middle");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-5, 0, 0));
			p.bt1 = transform(vec3(5, 0, 0));
			p.at2 = transform();
			p.bt2 = transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("tetrahedrons do not meet");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-5, 0, 0));
			p.bt1 = transform(vec3(5, 0, 0));
			p.at2 = transform(vec3(-5, 10, 0));
			p.bt2 = transform(vec3(-5, 0, 0));
			CAGE_TEST(!collisionDetection(p));
			CAGE_TEST(!p.fractionBefore.valid());
			CAGE_TEST(!p.fractionContact.valid());
			CAGE_TEST(p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("tetrahedrons are very close");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-1, 0, 0));
			p.bt1 = transform(vec3(1, 0, 0));
			p.at2 = transform();
			p.bt2 = transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("tetrahedrons are far apart");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-500, 0, 0));
			p.bt1 = transform(vec3(500, 0, 0));
			p.at2 = transform();
			p.bt2 = transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("one tetrahedron is very large");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-500, 0, 0), quat(), 50);
			p.bt1 = transform(vec3(5, 0, 0));
			p.at2 = transform(vec3(), quat(), 50);
			p.bt2 = transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("one tetrahedron is very small");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-5, 0, 0), quat(), 0.01);
			p.bt1 = transform(vec3(5, 0, 0));
			p.at2 = transform(vec3(), quat(), 0.01);
			p.bt2 = transform();
			CAGE_TEST(collisionDetection(p));
			CAGE_TEST(p.fractionBefore > 0 && p.fractionBefore < 1);
			CAGE_TEST(p.fractionContact > 0 && p.fractionContact < 1);
			CAGE_TEST(p.fractionBefore < p.fractionContact);
			CAGE_TEST(!p.collisionPairs.empty());
		}
		{
			CAGE_TESTCASE("no dynamic change (no collision)");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-5, 0, 0));
			p.bt1 = transform(vec3(5, 0, 0));
			p.at2 = transform(vec3(-5, 0, 0));
			p.bt2 = transform(vec3(5, 0, 0));
			CAGE_TEST(!collisionDetection(p));
		}
		{
			CAGE_TESTCASE("no dynamic change (with collision)");
			CollisionDetectionParams p(c1.get(), c1.get());
			p.at1 = transform(vec3(-0.5001, 0, 0));
			p.bt1 = transform(vec3(0.5, 0, 0));
			p.at2 = transform(vec3(-0.5, 0, 0));
			p.bt2 = transform(vec3(0.5001, 0, 0));
			CAGE_TEST(collisionDetection(p));
		}
	}

	{
		CAGE_TESTCASE("more collisions");
		Holder<Collider> c1 = newCollider();
		{ // tetrahedron 1
			const vec3 a(0, -0.7, 1);
			const vec3 b(+0.86603, -0.7, -0.5);
			const vec3 c(-0.86603, -0.7, -0.5);
			const vec3 d(0, 0.7, 0);
			const mat4 off = mat4(vec3(10, 0, 0));
			c1->addTriangle(Triangle(c, b, a) * off);
			c1->addTriangle(Triangle(a, b, d) * off);
			c1->addTriangle(Triangle(b, c, d) * off);
			c1->addTriangle(Triangle(c, a, d) * off);
			c1->rebuild();
		}
		Holder<Collider> c2 = newCollider();
		{ // tetrahedron 2
			const vec3 a(0, -0.7, 1);
			const vec3 b(+0.86603, -0.7, -0.5);
			const vec3 c(-0.86603, -0.7, -0.5);
			const vec3 d(0, 0.7, 0);
			const mat4 off = mat4(vec3(0, 10, 0));
			c2->addTriangle(Triangle(c, b, a) * off);
			c2->addTriangle(Triangle(a, b, d) * off);
			c2->addTriangle(Triangle(b, c, d) * off);
			c2->addTriangle(Triangle(c, a, d) * off);
			c2->rebuild();
		}
		{
			CAGE_TESTCASE("ensure static collision");
			{
				CollisionDetectionParams p(c1.get(), c2.get());
				CAGE_TEST(!collisionDetection(p));
			}
			{
				CollisionDetectionParams p(c1.get(), c2.get(), transform(vec3(-10, 9.5, 0)), transform());
				CAGE_TEST(collisionDetection(p));
			}
			{
				CollisionDetectionParams p(c1.get(), c2.get(), transform(), transform(vec3(), quat(degs(), degs(), degs(-90))));
				CAGE_TEST(collisionDetection(p));
			}
		}
		{
			CAGE_TESTCASE("rotations");
			{
				CollisionDetectionParams p(c1.get(), c2.get());
				p.bt1 = transform(vec3(), quat(degs(), degs(), degs(-100)));
				p.bt2 = transform(vec3(), quat(degs(), degs(), degs(-80)));
				CAGE_TEST(collisionDetection(p));
			}
			{
				CollisionDetectionParams p(c1.get(), c2.get());
				p.bt1 = transform(vec3(0.01, 0, 0), quat(degs(), degs(), degs(-80)));
				p.bt2 = transform(vec3(), quat(degs(), degs(), degs(-100)));
				CAGE_TEST(collisionDetection(p));
			}
		}
	}

	{
		CAGE_TESTCASE("randomized tests with triangles");

		Holder<Collider> tet = newCollider();
		{
			const vec3 a(0, -0.7, 1);
			const vec3 b(+0.86603, -0.7, -0.5);
			const vec3 c(-0.86603, -0.7, -0.5);
			const vec3 d(0, 0.7, 0);
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
				CAGE_LOG_THROW(stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const transform t1 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			const auto &trisTest = [&](const Triangle &l)
			{
				bool res = false;
				for (const Triangle t : tet->triangles())
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
			const vec3 a(0, -0.7, 1);
			const vec3 b(+0.86603, -0.7, -0.5);
			const vec3 c(-0.86603, -0.7, -0.5);
			const vec3 d(0, 0.7, 0);
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
				CAGE_LOG_THROW(stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const transform t1 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			const auto &trisTest = [&](const Line &l)
			{
				bool res = false;
				for (const Triangle t : tet->triangles())
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
