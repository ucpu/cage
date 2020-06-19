#include "main.h"
#include <cage-core/geometry.h>
#include <cage-core/collisionStructure.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>
#include <initializer_list>

void testCollisionStructure()
{
	CAGE_TESTCASE("collision structures");

	Holder<Collider> c1 = newCollider();
	{ // tetrahedron 1
		const vec3 a(0, -0.7, 1);
		const vec3 b(+0.86603, -0.7, -0.5);
		const vec3 c(-0.86603, -0.7, -0.5);
		const vec3 d(0, 0.7, 0);
		const transform off = transform(vec3(10, 0, 0));
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
		const transform off = transform(vec3(0, 10, 0));
		c2->addTriangle(triangle(c, b, a) * off);
		c2->addTriangle(triangle(a, b, d) * off);
		c2->addTriangle(triangle(b, c, d) * off);
		c2->addTriangle(triangle(c, a, d) * off);
		c2->rebuild();
	}
	Holder<Collider> c3 = newCollider();
	{ // tetrahedron 3
		const vec3 a(0, -0.7, 1);
		const vec3 b(+0.86603, -0.7, -0.5);
		const vec3 c(-0.86603, -0.7, -0.5);
		const vec3 d(0, 0.7, 0);
		c3->addTriangle(triangle(c, b, a));
		c3->addTriangle(triangle(a, b, d));
		c3->addTriangle(triangle(b, c, d));
		c3->addTriangle(triangle(c, a, d));
		c3->rebuild();
	}

	{
		CAGE_TESTCASE("tetrahedrons without transformations");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.get(), transform());
		data->update(2, c2.get(), transform());
		data->update(3, c3.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());

		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(c1.get(), transform()));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(c2.get(), transform()));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("3");
			CAGE_TEST(query->query(c3.get(), transform()));
			CAGE_TEST(query->name() == 3);
		}
		{
			CAGE_TESTCASE("none");
			CAGE_TEST(!query->query(c3.get(), transform(vec3(-10, 0, 0))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("31");
			CAGE_TEST(query->query(c3.get(), transform(vec3(9.5, 0, 0))));
			CAGE_TEST(query->name() == 1);
		}
	}

	{
		CAGE_TESTCASE("tetrahedrons with transformations in the structure");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c3.get(), transform(vec3(10, 0, 0)));
		data->update(2, c3.get(), transform(vec3(0, 10, 0)));
		data->update(3, c3.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());

		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(c1.get(), transform()));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(c2.get(), transform()));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("3");
			CAGE_TEST(query->query(c3.get(), transform()));
			CAGE_TEST(query->name() == 3);
		}
		{
			CAGE_TESTCASE("none");
			CAGE_TEST(!query->query(c3.get(), transform(vec3(-10, 0, 0))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("31");
			CAGE_TEST(query->query(c3.get(), transform(vec3(9.5, 0, 0))));
			CAGE_TEST(query->name() == 1);
		}
	}

	{
		CAGE_TESTCASE("tetrahedrons with transformations in queries");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.get(), transform());
		data->update(2, c2.get(), transform());
		data->update(3, c3.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());

		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(c3.get(), transform(vec3(10, 0, 0))));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(c3.get(), transform(vec3(0, 10, 0))));
			CAGE_TEST(query->name() == 2);
		}
	}

	{
		CAGE_TESTCASE("zero is valid name");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(0, c3.get(), transform());
		data->update(1, c1.get(), transform());
		data->update(2, c2.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());

		{
			CAGE_TESTCASE("0");
			CAGE_TEST(query->query(c3.get(), transform()));
			CAGE_TEST(query->name() == 0);
		}
		{
			CAGE_TESTCASE("1");
			CAGE_TEST(query->query(c1.get(), transform()));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("2");
			CAGE_TEST(query->query(c2.get(), transform()));
			CAGE_TEST(query->name() == 2);
		}
	}

	{
		CAGE_TESTCASE("simple shape-to-collider");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.get(), transform());
		data->update(2, c2.get(), transform());
		data->update(3, c3.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());
		{
			CAGE_TESTCASE("line 1");
			CAGE_TEST(query->query(makeSegment(vec3(-10, 0, 0), vec3(10, 0, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("line 2");
			CAGE_TEST(!query->query(makeSegment(vec3(-10, 0, 10), vec3(10, 0, 10))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("line 3");
			CAGE_TEST(query->query(makeLine(vec3(-1, 0, 0), vec3(1, 0, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("line 4");
			CAGE_TEST(!query->query(makeLine(vec3(-1, 0, 10), vec3(1, 0, 10))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("triangle 1");
			CAGE_TEST(query->query(triangle(vec3(10, -1, 1), vec3(10, 1, 1), vec3(10, 0, -2))));
			CAGE_TEST(query->name() == 1);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("triangle 2");
			CAGE_TEST(query->query(triangle(vec3(0, -1, 1), vec3(0, 1, 1), vec3(0, 0, -2))));
			CAGE_TEST(query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("triangle 3");
			CAGE_TEST(!query->query(triangle(vec3(10, -1, 11), vec3(10, 1, 11), vec3(10, 0, 8))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("plane 1");
			CAGE_TEST(query->query(plane(vec3(0,0,0), vec3(0, 1, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("plane 2");
			CAGE_TEST(!query->query(plane(vec3(0,0,10), vec3(0, 0, 1))));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("plane 3");
			CAGE_TEST(query->query(plane(vec3(123,0,456), vec3(0, 1, 0))));
			CAGE_TEST(query->name() == 1 || query->name() == 3);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("sphere 1");
			CAGE_TEST(query->query(sphere(vec3(0, 10, 0), 2)));
			CAGE_TEST(query->name() == 2);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("sphere 2");
			CAGE_TEST(!query->query(sphere(vec3(0, 10, 10), 2)));
			CAGE_TEST_ASSERTED(query->name());
		}
		{
			CAGE_TESTCASE("aabb");
			CAGE_TEST(query->query(aabb(vec3(-1, 9, -1), vec3(1, 11, 1))));
			CAGE_TEST(query->name() == 2);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
	}

	{
		CAGE_TESTCASE("first along line");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.get(), transform());
		data->update(2, c2.get(), transform());
		data->update(3, c3.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());
		// 2
		// 3 1

		{
			CAGE_TESTCASE("none");
			CAGE_TEST(!query->query(makeLine(vec3(1, -10, 0), vec3(0, -10, 0))));
		}
		{
			CAGE_TESTCASE("left 1");
			CAGE_TEST(query->query(makeLine(vec3(1, 0, 0), vec3())));
			CAGE_TEST(query->name() == 1);
		}
		{
			CAGE_TESTCASE("left 2");
			CAGE_TEST(query->query(makeLine(vec3(1, 10, 0), vec3(0, 10, 0))));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("right");
			CAGE_TEST(query->query(makeLine(vec3(-1, 0, 0), vec3())));
			CAGE_TEST(query->name() == 3);
		}
		{
			CAGE_TESTCASE("down 1");
			CAGE_TEST(query->query(makeLine(vec3(), vec3(0, -1, 0))));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("down 2");
			CAGE_TEST(query->query(makeLine(vec3(0, 1, 0), vec3())));
			CAGE_TEST(query->name() == 2);
		}
		{
			CAGE_TESTCASE("up");
			CAGE_TEST(query->query(makeLine(vec3(0, -1, 0), vec3())));
			CAGE_TEST(query->name() == 3);
		}
	}

	{
		CAGE_TESTCASE("first in time");

		Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
		data->update(1, c1.get(), transform());
		data->update(2, c2.get(), transform());
		data->update(3, c3.get(), transform());
		data->rebuild();
		Holder<CollisionQuery> query = newCollisionQuery(data.get());
		// 2
		// 3 1

		{
			CAGE_TESTCASE("left");
			CAGE_TEST(query->query(c3.get(), transform(vec3(100, 0, 0)), transform(vec3(-100, 0, 0))));
			CAGE_TEST(query->name() == 1);
			CAGE_TEST(query->collisionPairs().size() > 0);
		}
		{
			CAGE_TESTCASE("right");
			CAGE_TEST(query->query(c3.get(), transform(vec3(-100, 0, 0)), transform(vec3(100, 0, 0))));
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
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const transform t1 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
			data->update(3, c3.get(), t1);
			data->rebuild();
			Holder<CollisionQuery> query = newCollisionQuery(data.get());

			for (uint32 round = 0; round < 10; round++)
			{
				const line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				const bool res = intersects(l, c3.get(), t1);
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
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "A: " + attemptsA + ", B: " + attemptsB);
				CAGE_THROW_ERROR(Exception, "too many test attempts");
			}

			const transform t1 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);

			Holder<CollisionStructure> data = newCollisionStructure(CollisionStructureCreateConfig());
			data->update(3, c3.get(), t1);
			data->rebuild();
			Holder<CollisionQuery> query = newCollisionQuery(data.get());

			for (uint32 round = 0; round < 10; round++)
			{
				const transform t2 = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);
				for (const Collider *c : { c1.get(), c2.get(), c3.get() })
				{
					const bool res = intersects(c3.get(), c, t1, t2);
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
