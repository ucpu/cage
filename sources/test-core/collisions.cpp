#include "main.h"
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/collision.h>
#include <cage-core/collisionMesh.h>
#include <cage-core/memoryBuffer.h>

void testCollisions()
{
	{
		CAGE_TESTCASE("basic collisions");
		collisionPair pairs[10];
		{
			CAGE_TESTCASE("empty colliders");
			holder<collisionMesh> c1 = newCollisionMesh();
			holder<collisionMesh> c2 = newCollisionMesh();
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), transform(), transform(), pairs, 10) == 0);
		}
		{
			CAGE_TESTCASE("no collision");
			holder<collisionMesh> c1 = newCollisionMesh();
			holder<collisionMesh> c2 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->rebuild();
			c2->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), transform(), transform(), pairs, 10) == 0);
		}
		{
			CAGE_TESTCASE("one collision");
			holder<collisionMesh> c1 = newCollisionMesh();
			holder<collisionMesh> c2 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			c2->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c2.get(), transform(), transform(), pairs, 10) == 1);
			CAGE_TEST(pairs[0].a == 0);
			CAGE_TEST(pairs[0].b == 0);
		}
		{
			CAGE_TESTCASE("self collision");
			holder<collisionMesh> c1 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform(), pairs, 10) == 7);
		}
		{
			CAGE_TESTCASE("limited buffer");
			holder<collisionMesh> c1 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform(), pairs, 3) == 3);
		}
		{
			CAGE_TESTCASE("with transformation (no collision)");
			holder<collisionMesh> c1 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform(vec3(20, 0, 0)), pairs, 10) == 0);
		}
		{
			CAGE_TESTCASE("with transformation (one collision)");
			holder<collisionMesh> c1 = newCollisionMesh();
			holder<collisionMesh> c2 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c2->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c2->rebuild();
			c1->rebuild();
			CAGE_TEST(collisionDetection(c1.get(), c1.get(), transform(), transform(vec3(), quat(degs(), degs(90), degs())), pairs, 10) == 1);
		}
		{
			CAGE_TESTCASE("serialization");
			holder<collisionMesh> c1 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			memoryBuffer buff = c1->serialize();
			holder<collisionMesh> c2 = newCollisionMesh(buff);
			CAGE_TEST(c2->triangles().size() == 3);
			CAGE_TEST(c2->triangles()[2] == c1->triangles()[2]);
		}
		{
			CAGE_TESTCASE("forgot rebuilt");
			holder<collisionMesh> c1 = newCollisionMesh();
			c1->addTriangle(triangle(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0)));
			c1->addTriangle(triangle(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1)));
			c1->addTriangle(triangle(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0)));
			CAGE_TEST_ASSERTED(collisionDetection(c1.get(), c1.get(), transform(), transform(), pairs, 3));
		}
	}

	{
		CAGE_TESTCASE("dynamic collisions");
		holder<collisionMesh> c1 = newCollisionMesh();
		{ // tetraedr
			vec3 a(0, -0.7, 1);
			vec3 b(+0.86603, -0.7, -0.5);
			vec3 c(-0.86603, -0.7, -0.5);
			vec3 d(0, 0.7, 0);
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
			CAGE_TESTCASE("tetraeders meet in the middle");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0)), 
				transform(vec3(5, 0, 0)),
				transform(),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("tetraeders do not meet");
			CAGE_TEST(!collisionDetection(c1.get(), c1.get(),
				transform(vec3(-5, 0, 0)),
				transform(vec3(5, 0, 0)),
				transform(vec3(-5, 10, 0)),
				transform(vec3(-5, 0, 0)), fractionBefore, fractionContact));
			CAGE_TEST(!fractionContact.valid());
		}
		{
			CAGE_TESTCASE("tetraeders are very close");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-1, 0, 0)),
				transform(vec3(1, 0, 0)),
				transform(),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("tetraeders are far apart");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-500, 0, 0)),
				transform(vec3(500, 0, 0)),
				transform(),
				transform(),
				fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("one tetraedr is very large");
			CAGE_TEST(collisionDetection(c1.get(), c1.get(),
				transform(vec3(-500, 0, 0), quat(), 50),
				transform(vec3(5, 0, 0)),
				transform(vec3(), quat(), 50),
				transform(), fractionBefore, fractionContact));
			CAGE_TEST(fractionContact > 0 && fractionContact < 1);
		}
		{
			CAGE_TESTCASE("one tetraedr is very small");
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
		holder<collisionMesh> c1 = newCollisionMesh();
		{ // tetraedr 1
			vec3 a(0, -0.7, 1);
			vec3 b(+0.86603, -0.7, -0.5);
			vec3 c(-0.86603, -0.7, -0.5);
			vec3 d(0, 0.7, 0);
			mat4 off = mat4(vec3(10, 0, 0));
			c1->addTriangle(triangle(c, b, a) * off);
			c1->addTriangle(triangle(a, b, d) * off);
			c1->addTriangle(triangle(b, c, d) * off);
			c1->addTriangle(triangle(c, a, d) * off);
			c1->rebuild();
		}
		holder<collisionMesh> c2 = newCollisionMesh();
		{ // tetraedr 2
			vec3 a(0, -0.7, 1);
			vec3 b(+0.86603, -0.7, -0.5);
			vec3 c(-0.86603, -0.7, -0.5);
			vec3 d(0, 0.7, 0);
			mat4 off = mat4(vec3(0, 10, 0));
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
		CAGE_TESTCASE("collisions grid");
		holder<collisionMesh> c1 = newCollisionMesh();
		{ // tetraedr 1
			vec3 a(0, -0.7, 1);
			vec3 b(+0.86603, -0.7, -0.5);
			vec3 c(-0.86603, -0.7, -0.5);
			vec3 d(0, 0.7, 0);
			mat4 off = mat4(vec3(10, 0, 0));
			c1->addTriangle(triangle(c, b, a) * off);
			c1->addTriangle(triangle(a, b, d) * off);
			c1->addTriangle(triangle(b, c, d) * off);
			c1->addTriangle(triangle(c, a, d) * off);
			c1->rebuild();
		}
		holder<collisionMesh> c2 = newCollisionMesh();
		{ // tetraedr 2
			vec3 a(0, -0.7, 1);
			vec3 b(+0.86603, -0.7, -0.5);
			vec3 c(-0.86603, -0.7, -0.5);
			vec3 d(0, 0.7, 0);
			mat4 off = mat4(vec3(0, 10, 0));
			c2->addTriangle(triangle(c, b, a) * off);
			c2->addTriangle(triangle(a, b, d) * off);
			c2->addTriangle(triangle(b, c, d) * off);
			c2->addTriangle(triangle(c, a, d) * off);
			c2->rebuild();
		}
		holder<collisionMesh> c3 = newCollisionMesh();
		{ // tetraedr 3
			vec3 a(0, -0.7, 1);
			vec3 b(+0.86603, -0.7, -0.5);
			vec3 c(-0.86603, -0.7, -0.5);
			vec3 d(0, 0.7, 0);
			c3->addTriangle(triangle(c, b, a));
			c3->addTriangle(triangle(a, b, d));
			c3->addTriangle(triangle(b, c, d));
			c3->addTriangle(triangle(c, a, d));
			c3->rebuild();
		}
		{
			CAGE_TESTCASE("data without transformations");
			holder<collisionData> data = newCollisionData(collisionDataCreateConfig());
			data->update(1, c1.get(), transform());
			data->update(2, c2.get(), transform());
			data->update(3, c3.get(), transform());
			data->rebuild();
			holder<collisionQuery> query = newCollisionQuery(data.get());
			{
				CAGE_TESTCASE("1");
				query->query(c1.get(), transform());
				CAGE_TEST(query->name() == 1);
				CAGE_TEST(query->fractionContact() == 0);
			}
			{
				CAGE_TESTCASE("2");
				query->query(c2.get(), transform());
				CAGE_TEST(query->name() == 2);
				CAGE_TEST(query->fractionContact() == 0);
			}
			{
				CAGE_TESTCASE("3");
				query->query(c3.get(), transform());
				CAGE_TEST(query->name() == 3);
				CAGE_TEST(query->fractionContact() == 0);
			}
			{
				CAGE_TESTCASE("none");
				query->query(c3.get(), transform(vec3(-10, 0, 0)));
				CAGE_TEST(query->name() == 0);
				CAGE_TEST(!query->fractionContact().valid());
			}
			{
				CAGE_TESTCASE("31");
				query->query(c3.get(), transform(vec3(9.5, 0, 0)));
				CAGE_TEST(query->name() == 1);
				CAGE_TEST(query->fractionContact() == 0);
			}
		}
		{
			CAGE_TESTCASE("shape-to-collider collision");
			holder<collisionData> data = newCollisionData(collisionDataCreateConfig());
			data->update(1, c1.get(), transform());
			data->update(2, c2.get(), transform());
			data->update(3, c3.get(), transform());
			data->rebuild();
			holder<collisionQuery> query = newCollisionQuery(data.get());
			{
				CAGE_TESTCASE("line 1");
				query->query(makeSegment(vec3(-10, 0, 0), vec3(10, 0, 0)));
				CAGE_TEST(query->name() == 1 || query->name() == 3);
			}
			{
				CAGE_TESTCASE("triangle 1");
				query->query(triangle(vec3(10, -1, 1), vec3(10, 1, 1), vec3(10, 0, -2)));
				CAGE_TEST(query->name() == 1);
			}
			{
				CAGE_TESTCASE("triangle 2");
				query->query(triangle(vec3(0, -1, 1), vec3(0, 1, 1), vec3(0, 0, -2)));
				CAGE_TEST(query->name() == 3);
			}
			{
				CAGE_TESTCASE("plane");
				query->query(plane(vec3(0,0,0), vec3(0, 1, 0)));
				CAGE_TEST(query->name() == 1 || query->name() == 3);
			}
			{
				CAGE_TESTCASE("sphere");
				query->query(sphere(vec3(0, 10, 0), 2));
				CAGE_TEST(query->name() == 2);
			}
			{
				CAGE_TESTCASE("aabb");
				query->query(aabb(vec3(-1, 9, -1), vec3(1, 11, 1)));
				CAGE_TEST(query->name() == 2);
			}
		}
	}
}
