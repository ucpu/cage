#include <initializer_list>

#include "main.h"

#include <cage-core/collider.h>
#include <cage-core/collisionStructure.h>
#include <cage-core/geometry.h>
#include <cage-core/memoryBuffer.h>

void testCollisionStructure()
{
	CAGE_TESTCASE("collision structures");

	Holder<Collider> c1 = newCollider();
	{ // tetrahedron 1
		const Vec3 a(0, -0.7, 1);
		const Vec3 b(+0.86603, -0.7, -0.5);
		const Vec3 c(-0.86603, -0.7, -0.5);
		const Vec3 d(0, 0.7, 0);
		const Transform off = Transform(Vec3(10, 0, 0));
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
		const Transform off = Transform(Vec3(0, 10, 0));
		c2->addTriangle(Triangle(c, b, a) * off);
		c2->addTriangle(Triangle(a, b, d) * off);
		c2->addTriangle(Triangle(b, c, d) * off);
		c2->addTriangle(Triangle(c, a, d) * off);
		c2->rebuild();
	}
	Holder<Collider> c3 = newCollider();
	{ // tetrahedron 3
		const Vec3 a(0, -0.7, 1);
		const Vec3 b(+0.86603, -0.7, -0.5);
		const Vec3 c(-0.86603, -0.7, -0.5);
		const Vec3 d(0, 0.7, 0);
		c3->addTriangle(Triangle(c, b, a));
		c3->addTriangle(Triangle(a, b, d));
		c3->addTriangle(Triangle(b, c, d));
		c3->addTriangle(Triangle(c, a, d));
		c3->rebuild();
	}

	{
		CAGE_TESTCASE("tetrahedrons without transformations");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.share(), Transform());
		data->update(2, c2.share(), Transform());
		data->update(3, c3.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());

		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(+c1, Transform()));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(+c2, Transform()));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("3");
			CAGE_TEST(query->query(+c3, Transform()));
			CAGE_TEST(query->name() == 3);
		}
		{
			CAGE_TESTCASE("none");
			CAGE_TEST(!query->query(+c3, Transform(Vec3(-10, 0, 0))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("31");
			CAGE_TEST(query->query(+c3, Transform(Vec3(9.5, 0, 0))));
			CAGE_TEST(query->name() == 1);
		}
	}

	{
		CAGE_TESTCASE("tetrahedrons with transformations in the structure");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c3.share(), Transform(Vec3(10, 0, 0)));
		data->update(2, c3.share(), Transform(Vec3(0, 10, 0)));
		data->update(3, c3.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());

		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(+c1, Transform()));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(+c2, Transform()));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("3");
			CAGE_TEST(query->query(+c3, Transform()));
			CAGE_TEST(query->name() == 3);
		}
		{
			CAGE_TESTCASE("none");
			CAGE_TEST(!query->query(+c3, Transform(Vec3(-10, 0, 0))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("31");
			CAGE_TEST(query->query(+c3, Transform(Vec3(9.5, 0, 0))));
			CAGE_TEST(query->name() == 1);
		}
	}

	{
		CAGE_TESTCASE("tetrahedrons with transformations in queries");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.share(), Transform());
		data->update(2, c2.share(), Transform());
		data->update(3, c3.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());

		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(+c3, Transform(Vec3(10, 0, 0))));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(+c3, Transform(Vec3(0, 10, 0))));
			CAGE_TEST(query->name() == 2);
		}
	}

	{
		CAGE_TESTCASE("zero is valid name");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(0, c3.share(), Transform());
		data->update(1, c1.share(), Transform());
		data->update(2, c2.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());

		{
			CAGE_TESTCASE("0");
			CAGE_TEST(query->query(+c3, Transform()));
			CAGE_TEST(query->name() == 0);
		}
		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(+c1, Transform()));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(+c2, Transform()));
			CAGE_TEST(query->name() == 2);
		}
	}

	{
		CAGE_TESTCASE("simple shape-to-collider");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.share(), Transform());
		data->update(2, c2.share(), Transform());
		data->update(3, c3.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());
		{
			CAGE_TESTCASE("line 1");
			CAGE_TEST(query->query(makeSegment(Vec3(-10, 0, 0), Vec3(10, 0, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("line 2");
			CAGE_TEST(!query->query(makeSegment(Vec3(-10, 0, 10), Vec3(10, 0, 10))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("line 3");
			CAGE_TEST(query->query(makeLine(Vec3(-1, 0, 0), Vec3(1, 0, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("line 4");
			CAGE_TEST(!query->query(makeLine(Vec3(-1, 0, 10), Vec3(1, 0, 10))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("triangle 1");
			CAGE_TEST(query->query(Triangle(Vec3(10, -1, 1), Vec3(10, 1, 1), Vec3(10, 0, -2))));
			CAGE_TEST(query->name() == 1);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("triangle 2");
			CAGE_TEST(query->query(Triangle(Vec3(0, -1, 1), Vec3(0, 1, 1), Vec3(0, 0, -2))));
			CAGE_TEST(query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("triangle 3");
			CAGE_TEST(!query->query(Triangle(Vec3(10, -1, 11), Vec3(10, 1, 11), Vec3(10, 0, 8))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("plane 1");
			CAGE_TEST(query->query(Plane(Vec3(0, 0, 0), Vec3(0, 1, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("plane 2");
			CAGE_TEST(!query->query(Plane(Vec3(0, 0, 10), Vec3(0, 0, 1))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("plane 3");
			CAGE_TEST(query->query(Plane(Vec3(123, 0, 456), Vec3(0, 1, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("sphere 1");
			CAGE_TEST(query->query(Sphere(Vec3(0, 10, 0), 2)));
			CAGE_TEST(query->name() == 2);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("sphere 2");
			CAGE_TEST(!query->query(Sphere(Vec3(0, 10, 10), 2)));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("aabb");
			CAGE_TEST(query->query(Aabb(Vec3(-1, 9, -1), Vec3(1, 11, 1))));
			CAGE_TEST(query->name() == 2);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
	}

	{
		CAGE_TESTCASE("first along line");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.share(), Transform());
		data->update(2, c2.share(), Transform());
		data->update(3, c3.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());
		// 2
		// 3 1

		{
			CAGE_TESTCASE("none");
			CAGE_TEST(!query->query(makeLine(Vec3(1, -10, 0), Vec3(0, -10, 0))));
		}
		{
			CAGE_TESTCASE("left 1");
			CAGE_TEST(query->query(makeLine(Vec3(1, 0, 0), Vec3())));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("left 2");
			CAGE_TEST(query->query(makeLine(Vec3(1, 10, 0), Vec3(0, 10, 0))));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("right");
			CAGE_TEST(query->query(makeLine(Vec3(-1, 0, 0), Vec3())));
			CAGE_TEST(query->name() == 3);
		}
		{
			CAGE_TESTCASE("down 1");
			CAGE_TEST(query->query(makeLine(Vec3(), Vec3(0, -1, 0))));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("down 2");
			CAGE_TEST(query->query(makeLine(Vec3(0, 1, 0), Vec3())));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("up");
			CAGE_TEST(query->query(makeLine(Vec3(0, -1, 0), Vec3())));
			CAGE_TEST(query->name() == 3);
		}
	}

	{
		CAGE_TESTCASE("first in time");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.share(), Transform());
		data->update(2, c2.share(), Transform());
		data->update(3, c3.share(), Transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.share());
		// 2
		// 3 1

		{
			CAGE_TESTCASE("left");
			CAGE_TEST(query->query(+c3, Transform(Vec3(100, 0, 0)), Transform(Vec3(-100, 0, 0))));
			CAGE_TEST(query->name() == 1);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("right");
			CAGE_TEST(query->query(+c3, Transform(Vec3(-100, 0, 0)), Transform(Vec3(100, 0, 0))));
			CAGE_TEST(query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
	}

	{
		CAGE_TESTCASE("shapes with randomized transformations");

		uint32 attemptsA = 0, attemptsB = 0;
		while (attemptsA < 5 || attemptsB < 5)
		{
			if (attemptsA + attemptsB > 1000)
			{
				CAGE_LOG_THROW(Stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const Transform t1 = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
			data->update(3, c3.share(), t1);
			data->rebuild();
			Holder<CollisionQuery> query = newCollisionQuery(data.share());

			for (uint32 round = 0; round < 10; round++)
			{
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				const bool res = intersects(l, +c3, t1);
				CAGE_TEST(query->query(l) == res);
				if (res)
				{
					CAGE_TEST(query->name() == 3);
					attemptsA++;
				}
				else
					attemptsB++;
			}
		}
	}

	{
		CAGE_TESTCASE("tetrahedrons with randomized transformations");

		uint32 attemptsA = 0, attemptsB = 0;
		while (attemptsA < 5 || attemptsB < 5)
		{
			if (attemptsA + attemptsB > 1000)
			{
				CAGE_LOG_THROW(Stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const Transform t1 = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
			data->update(3, c3.share(), t1);
			data->rebuild();
			Holder<CollisionQuery> query = newCollisionQuery(data.share());

			for (uint32 round = 0; round < 10; round++)
			{
				const Transform t2 = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);
				for (const Collider *c : { +c1, +c2, +c3 })
				{
					const bool res = intersects(+c3, c, t1, t2);
					CAGE_TEST(query->query(c, t2) == res);
					if (res)
					{
						CAGE_TEST(query->name() == 3);
						attemptsA++;
					}
					else
						attemptsB++;
				}
			}
		}
	}
}
