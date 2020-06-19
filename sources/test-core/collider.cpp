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
			CAGE_TEST(!collisionDetection(c1.get(), c2.get(), transform(), transform()));
		}
		{
			CAGE_TESTCASE("no collision");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->rebuild();
			c2->rebuild();
			CAGE_TEST(!collisionDetection(c1.get(), c2.get(), transform(), transform()));
		}
		{
			CAGE_TESTCASE("one collision");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			c2->rebuild();
			Holder<PointerRange<CollisionPair>> pairs;
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), transform(), transform(), pairs));
			CAGE_TEST(pairs.size() == 1);
			CAGE_TEST(pairs[0].a == 0);
			CAGE_TEST(pairs[0].b == 0);
		}
		{
			CAGE_TESTCASE("self collision");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform()));
		}
		{
			CAGE_TESTCASE("limited buffer");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform()));
		}
		{
			CAGE_TESTCASE("with transformation (no collision)");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CAGE_TEST(!collisionDetection(c1.get(), c1.get(), transform(), transform(vec3(20, 0, 0))));
		}
		{
			CAGE_TESTCASE("with transformation (one collision)");
			Holder<Collider> c1 = newCollider();
			Holder<Collider> c2 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c2->rebuild();
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform(vec3(), quat(degs(), degs(90), degs()))));
		}
		{
			CAGE_TESTCASE("serialization");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			MemoryBuffer buff = c1->serialize();
			Holder<Collider> c2 = newCollider();
			c2->deserialize(buff);
			CAGE_TEST(c2->triangles().size() == 3);
			CAGE_TEST(c2->triangles()[2] == c1->triangles()[2]);
		}
		{
			CAGE_TESTCASE("forgot rebuilt");
			Holder<Collider> c1 = newCollider();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			CAGE_TEST_ASSERTED(collisionDetection(c1.get(), c1.get(), transform(), transform()));
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
			c1->addTriangle(triangle(c, b, a));
			c1->addTriangle(triangle(a, b, d));
			c1->addTriangle(triangle(b, c, d));
			c1->addTriangle(triangle(c, a, d));
			c1->rebuild();
		}
		{
			CAGE_TESTCASE("ensure static collision");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0), quat(), 7),
				transform(vec3(5, 0, 0), quat(), 7)));
		}
		real fractionBefore, fractionContact;
		{
			CAGE_TESTCASE("tetrahedrons meet in the middle");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0)), 
				transform(vec3(5, 0, 0)),
				transform(),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("tetrahedrons do not meet");
			CAGE_TEST(!collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0)),
				transform(vec3(5, 0, 0)),
				transform(vec3(-5, 10, 0)),
				transform(vec3(-5, 0, 0)), fractionBefore, fractionContact));
			CAGE_TEST(!fractionContact.valid());
		}
		{
			CAGE_TESTCASE("tetrahedrons are very close");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-1, 0, 0)),
				transform(vec3(1, 0, 0)),
				transform(),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("tetrahedrons are far apart");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-500, 0, 0)),
				transform(vec3(500, 0, 0)),
				transform(),
				transform(),
				fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("one tetrahedron is very large");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-500, 0, 0), quat(), 50),
				transform(vec3(5, 0, 0)),
				transform(vec3(), quat(), 50),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("one tetrahedron is very small");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0), quat(), 0.01),
				transform(vec3(5, 0, 0)),
				transform(vec3(), quat(), 0.01),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("no dynamic change (no collision)");
			CAGE_TEST(!collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0)),
				transform(vec3(5, 0, 0)),
				transform(vec3(-5, 0, 0)),
				transform(vec3(5, 0, 0)), fractionBefore, fractionContact));
			CAGE_TEST(!fractionContact.valid());
		}
		{
			CAGE_TESTCASE("no dynamic change (with collision)");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), 
				transform(vec3(-0.5001, 0, 0)),
				transform(vec3(0.5, 0, 0)), 
				transform(vec3(-0.5, 0, 0)), 
				transform(vec3(0.5001, 0, 0)), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact.valid());
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
			c1->addTriangle(triangle(c, b, a) * off);
			c1->addTriangle(triangle(a, b, d) * off);
			c1->addTriangle(triangle(b, c, d) * off);
			c1->addTriangle(triangle(c, a, d) * off);
			c1->rebuild();
		}
		Holder<Collider> c2 = newCollider();
		{ // tetrahedron 2
			const vec3 a(0, -0.7, 1);
			const vec3 b(+0.86603, -0.7, -0.5);
			const vec3 c(-0.86603, -0.7, -0.5);
			const vec3 d(0, 0.7, 0);
			const mat4 off = mat4(vec3(0, 10, 0));
			c2->addTriangle(triangle(c, b, a) * off);
			c2->addTriangle(triangle(a, b, d) * off);
			c2->addTriangle(triangle(b, c, d) * off);
			c2->addTriangle(triangle(c, a, d) * off);
			c2->rebuild();
		}
		{
			CAGE_TESTCASE("ensure static collision");
			CAGE_TEST(!collisionDetection(c1.get(), c2.get(),
				transform(), 
				transform()));
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), 
				transform(vec3(0, 9.5, 0)), 
				transform(vec3(9.5, 0, 0))));
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), 
				transform(), 
				transform(vec3(), quat(degs(), degs(), degs(-90)))));
		}
		{
			CAGE_TESTCASE("rotations");
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), 
				transform(), 
				transform(vec3(), quat(degs(), degs(), degs(-100))),
				transform(),
				transform(vec3(), quat(degs(), degs(), degs(-80)))));
			CAGE_TEST(collisionDetection(c1.get(), c2.get(),
				transform(),
				transform(vec3(0.01, 0, 0), quat(degs(), degs(), degs(-80))), 
				transform(), 
				transform(vec3(), quat(degs(), degs(), degs(-100)))));
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
			tet->addTriangle(triangle(c, b, a));
			tet->addTriangle(triangle(a, b, d));
			tet->addTriangle(triangle(b, c, d));
			tet->addTriangle(triangle(c, a, d));
			tet->rebuild();
		}

		uint32 attemptsA = 0, attemptsB = 0;
		while (attemptsA < 5 || attemptsB < 5)
		{
			if (attemptsA + attemptsB > 1000)
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const transform t1 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			const auto &trisTest = [&](const triangle &l)
			{
				bool res = false;
				for (const triangle t : tet->triangles())
					res = res || intersects(l, t * t1);
				return res;
			};

			for (uint32 round = 0; round < 10; round++)
			{
				const triangle l = triangle(randomDirection3() * 10, randomDirection3() * 10, randomDirection3() * 10);
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
			tet->addTriangle(triangle(c, b, a));
			tet->addTriangle(triangle(a, b, d));
			tet->addTriangle(triangle(b, c, d));
			tet->addTriangle(triangle(c, a, d));
			tet->rebuild();
		}

		uint32 attemptsA = 0, attemptsB = 0;
		while (attemptsA < 5 || attemptsB < 5)
		{
			if (attemptsA + attemptsB > 1000)
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const transform t1 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			const auto &trisTest = [&](const line &l)
			{
				bool res = false;
				for (const triangle t : tet->triangles())
					res = res || intersects(l, t * t1);
				return res;
			};

			for (uint32 round = 0; round < 10; round++)
			{
				const line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
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
