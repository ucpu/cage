#include "main.h"
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/geometry.h>

#include <cmath>

void test(real a, real b);
void test(rads a, rads b);
void test(const quat &a, const quat &b);
void test(const vec3 &a, const vec3 &b);
void test(const vec4 &a, const vec4 &b);
void test(const mat3 &a, const mat3 &b);
void test(const mat4 &a, const mat4 &b);
void test(const Aabb &a, const Aabb &b)
{
	test(a.a, b.a);
	test(a.b, b.b);
}
void test(const Sphere &a, const Sphere &b)
{
	test(a.center, b.center);
	test(a.radius, b.radius);
}

namespace
{
	void testEx(real a, real b)
	{
		if (!a.finite() || !b.finite())
		{
			// test without epsilon
			CAGE_TEST(a == b);
		}
		else
			test(a, b);
	}

	void testEx(const vec3 &a, const vec3 &b)
	{
		testEx(a[0], b[0]);
		testEx(a[1], b[1]);
		testEx(a[2], b[2]);
	}
}

void testGeometry()
{
	CAGE_TESTCASE("geometry");

	{
		CAGE_TESTCASE("points");

		CAGE_TEST(intersects(vec3(1, 2, 3), vec3(1, 2, 3)));
		CAGE_TEST(!intersects(vec3(1, 2, 3), vec3(3, 2, 1)));

		test(distance(vec3(1, 2, 3), vec3(1, 2, 3)), 0);
		test(distance(vec3(1, 2, 3), vec3(3, 2, 1)), sqrt(8));
	}

	{
		CAGE_TESTCASE("lines");

		{
			CAGE_TESTCASE("normalization");

			Line l = makeSegment(vec3(1, 2, 3), vec3(3, 4, 5));
			CAGE_TEST(!l.isPoint() && l.isSegment() && !l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());
			test(l.direction, normalize(vec3(3, 4, 5) - vec3(1, 2, 3)));

			l = makeRay(vec3(1, 2, 3), vec3(3, 4, 5));
			CAGE_TEST(!l.isPoint() && !l.isSegment() && l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());
			test(l.direction, normalize(vec3(3, 4, 5) - vec3(1, 2, 3)));

			l = makeLine(vec3(1, 2, 3), vec3(3, 4, 5));
			CAGE_TEST(!l.isPoint() && !l.isSegment() && !l.isRay() && l.isLine());
			CAGE_TEST(l.normalized());
			test(l.direction, normalize(vec3(3, 4, 5) - vec3(1, 2, 3)));

			l = Line(vec3(1, 2, 3), vec3(3, 4, 5), -5, 5);
			CAGE_TEST(l.valid());
			CAGE_TEST(!l.normalized());

			l = makeRay(vec3(1, 2, 3), vec3(1, 2, 3));
			CAGE_TEST(l.isPoint() && l.isSegment() && !l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());

			l = makeSegment(vec3(-5, 0, 0), vec3(5, 0, 0));
			CAGE_TEST(!l.isPoint() && l.isSegment() && !l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());
			test(l.minimum, 0);
			test(l.maximum, 10);
			test(l.a(), vec3(-5, 0, 0));
			test(l.b(), vec3(5, 0, 0));

			l = makeSegment(vec3(0.1, 0, 16), vec3(0.1, 0, 12));
			CAGE_TEST(l.normalized());
			test(l.origin, vec3(0.1, 0, 16));
			test(l.direction, vec3(0, 0, -1));
			test(l.minimum, 0);
			test(l.maximum, 4);
		}

		{
			CAGE_TESTCASE("distances (segments)");

			const Line a = makeSegment(vec3(0, -1, 0), vec3(0, 1, 0));
			const Line b = makeSegment(vec3(1, -2, 0), vec3(1, 2, 0));
			const Line c = makeSegment(vec3(0, 0, 0), vec3(0, 0, 1));
			const Line d = makeSegment(vec3(3, -1, 0), vec3(3, 1, 0));
			test(distance(a, a), 0);
			test(distance(a, b), 1);
			test(distance(b, a), 1);
			test(distance(a, c), 0);
			test(distance(b, c), 1);
			test(distance(a, d), 3);
		}

		{
			CAGE_TESTCASE("distances (lines)");

			test(distance(makeLine(vec3(), vec3(0, 0, 1)), makeLine(vec3(0, 0, 1), vec3())), 0);
			test(distance(makeLine(vec3(), vec3(0, 0, 1)), makeLine(vec3(), vec3(0, 1, 0))), 0);
			test(distance(makeLine(vec3(), vec3(0, 0, 1)), makeLine(vec3(1, 0, 1), vec3(1, 0, 0))), 1);
			test(distance(makeLine(vec3(), vec3(0, 0, 1)), makeLine(vec3(1, 0, 0), vec3(1, 1, 0))), 1);
		}

		{
			CAGE_TESTCASE("angles");

			const Line a = makeSegment(vec3(0, -1, 1), vec3(0, 1, 1));
			const Line b = makeSegment(vec3(-1, 0, 2), vec3(1, 0, 2));
			const Line c = makeSegment(vec3(0, 0, 0), vec3(1, 1, 1));
			const Line d = makeSegment(vec3(3, -1, 0), vec3(3, 1, 0));
			test(angle(a, a), degs(0));
			test(angle(a, b), degs(90));
			test(angle(b, a), degs(90));
			test(angle(a, c), atan2(real(1), sqrt(2)));
		}

		{
			CAGE_TESTCASE("randomized intersects");

			for (uint32 round = 0; round < 10; round++)
			{
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				CAGE_TEST(intersects(l, l.origin));
				CAGE_TEST(intersects(l, l.origin + l.direction));
				CAGE_TEST(intersects(l, l.origin - l.direction));
				CAGE_TEST(!intersects(l, l.origin + vec3(1, 0, 0)));
			}
		}

		{
			CAGE_TESTCASE("randomized transformations");

			for (uint32 round = 0; round < 5; round++)
			{
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				const vec3 p = l.origin + l.direction * (randomChance() + 1);
				CAGE_TEST(intersects(l, p));
				for (uint32 round2 = 0; round2 < 5; round2++)
				{
					const transform tr = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() + 1);
					const Line lt = l * tr;
					const vec3 pt = p * tr;
					CAGE_TEST(intersects(lt, pt));
				}
			}
		}
	}

	{
		CAGE_TESTCASE("triangles");

		{
			CAGE_TESTCASE("basics");

			Triangle t1(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0));
			const Triangle t2(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1));
			const Triangle t3(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0));
			CAGE_TEST(!intersects(t1, t2));
			CAGE_TEST(intersects(t1, t3));
			CAGE_TEST(intersects(t2, t3));
			t1.area();
			t1.center();
			t1.normal();
			t1 *= mat4(vec3(1, 2, 3));
			t1 *= mat4(vec3(4, 5, 10), quat(degs(), degs(42), degs()), vec3(3, 2, 1));
		}

		{
			CAGE_TESTCASE("tests");

			const Triangle t1(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0));
			test(t1.normal(), -t1.flip().normal());
			CAGE_TEST(!t1.degenerated());
			CAGE_TEST(Triangle(vec3(1, 2, 3), vec3(3, 2, 1), vec3(1, 2, 3)).degenerated()); // two vertices are the same
			CAGE_TEST(Triangle(vec3(1, 2, 3), vec3(2, 4, 6), vec3(3, 6, 9)).degenerated()); // vertices are collinear
		}

		{
			CAGE_TESTCASE("closest point");

			const Triangle tri = Triangle(vec3(), vec3(2, 0, 0), vec3(0, 2, 0));
			test(closestPoint(tri, vec3()), vec3());
			test(closestPoint(tri, vec3(2, 0, 0)), vec3(2, 0, 0));
			test(closestPoint(tri, vec3(0, 2, 0)), vec3(0, 2, 0));
			test(closestPoint(tri, vec3(1, 1, 0)), vec3(1, 1, 0));
			test(closestPoint(tri, vec3(2, 2, 0)), vec3(1, 1, 0));
			test(closestPoint(tri, vec3(1.5, 1.5, 0)), vec3(1, 1, 0));
			test(closestPoint(tri, vec3(0.5, 0.5, 0)), vec3(0.5, 0.5, 0));
			test(closestPoint(tri, vec3(1, 0, 0)), vec3(1, 0, 0));
			test(closestPoint(tri, vec3(0, 1, 0)), vec3(0, 1, 0));
			test(closestPoint(tri, vec3(5, 0, 0)), vec3(2, 0, 0));
			test(closestPoint(tri, vec3(0, 5, 0)), vec3(0, 2, 0));
			test(closestPoint(tri, vec3(-5, 0, 0)), vec3());
			test(closestPoint(tri, vec3(0, -5, 0)), vec3());
			test(closestPoint(tri, vec3(-5, -5, 0)), vec3());
			test(closestPoint(tri, vec3(5, -5, 0)), vec3(2, 0, 0));
			test(closestPoint(tri, vec3(-5, 5, 0)), vec3(0, 2, 0));
			test(closestPoint(tri, vec3(1, -5, 0)), vec3(1, 0, 0));
			test(closestPoint(tri, vec3(-5, 1, 0)), vec3(0, 1, 0));
			test(closestPoint(vec3(-5, 1, 0), tri), vec3(0, 1, 0));
		}

		{
			CAGE_TESTCASE("randomized closest point");

			for (uint32 round = 0; round < 10; round++)
			{
				const transform tr = transform(randomDirection3() * randomChance() * 10, randomDirectionQuat());
				const Triangle tri = Triangle(vec3(), vec3(2, 0, 0), vec3(0, 2, 0)) * tr;
				const real z = randomRange(-100, 100);
				test(closestPoint(tri, vec3() * tr), vec3() * tr);
				test(closestPoint(tri, vec3(2, 0, z) * tr), vec3(2, 0, 0) * tr);
				test(closestPoint(tri, vec3(0, 2, z) * tr), vec3(0, 2, 0) * tr);
				test(closestPoint(tri, vec3(1, 1, z) * tr), vec3(1, 1, 0) * tr);
				test(closestPoint(tri, vec3(2, 2, z) * tr), vec3(1, 1, 0) * tr);
				test(closestPoint(tri, vec3(1.5, 1.5, z) * tr), vec3(1, 1, 0) * tr);
				test(closestPoint(tri, vec3(0.5, 0.5, z) * tr), vec3(0.5, 0.5, 0) * tr);
				test(closestPoint(tri, vec3(1, 0, z) * tr), vec3(1, 0, 0) * tr);
				test(closestPoint(tri, vec3(0, 1, z) * tr), vec3(0, 1, 0) * tr);
				test(closestPoint(tri, vec3(5, 0, z) * tr), vec3(2, 0, 0) * tr);
				test(closestPoint(tri, vec3(0, 5, z) * tr), vec3(0, 2, 0) * tr);
				test(closestPoint(tri, vec3(-5, 0, z) * tr), vec3() * tr);
				test(closestPoint(tri, vec3(0, -5, z) * tr), vec3() * tr);
				test(closestPoint(tri, vec3(-5, -5, z) * tr), vec3() * tr);
				test(closestPoint(tri, vec3(5, -5, z) * tr), vec3(2, 0, 0) * tr);
				test(closestPoint(tri, vec3(-5, 5, z) * tr), vec3(0, 2, 0) * tr);
				test(closestPoint(tri, vec3(1, -5, z) * tr), vec3(1, 0, 0) * tr);
				test(closestPoint(tri, vec3(-5, 1, z) * tr), vec3(0, 1, 0) * tr);
			}
		}

		{
			CAGE_TESTCASE("intersections (with lines)");

			const Triangle t1(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0));
			const Triangle t2(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1));
			const Triangle t3(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0));
			test(intersection(makeSegment(vec3(0, 1, -1), vec3(0, 1, 1)), t1), vec3(0, 1, 0));
			test(intersection(t1, makeSegment(vec3(0, 1, -1), vec3(0, 1, 1))), vec3(0, 1, 0));
			CAGE_TEST(intersects(makeSegment(vec3(0, 1, -1), vec3(0, 1, 1)), t1));
			CAGE_TEST(intersects(t1, makeSegment(vec3(0, 1, -1), vec3(0, 1, 1))));
			CAGE_TEST(!intersection(makeSegment(vec3(5, 1, -1), vec3(0, 1, 1)), t1).valid());
			CAGE_TEST(!intersects(makeSegment(vec3(5, 1, -1), vec3(0, 1, 1)), t1));
			test(intersection(makeSegment(vec3(2, 5, 0), vec3(2, -5, 0)), t3), vec3(2, 1, 0));
			CAGE_TEST(intersects(makeSegment(vec3(2, 5, 0), vec3(2, -5, 0)), t3));
			CAGE_TEST(!intersection(makeSegment(vec3(2, 0, 0), vec3(2, -5, 0)), t3).valid());
			CAGE_TEST(!intersects(makeSegment(vec3(2, 0, 0), vec3(2, -5, 0)), t3));
		}

		{
			CAGE_TESTCASE("distances (with lines)");

			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), makeSegment(vec3(-1, -1, -2), vec3(-1, -1, 2))), 0);
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), makeSegment(vec3(1, 1, -2), vec3(1, 1, 2))), cage::sqrt(2));
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), makeSegment(vec3(-5, 0, 1), vec3(5, 0, 1))), 1);
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), makeSegment(vec3(-1.5, 0, 1), vec3(-0.5, 0, 1))), 1);
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), makeSegment(vec3(-1.5, -6, 3), vec3(-0.5, -6, 3))), 5);
			test(distance(makeSegment(vec3(-1.5, -6, 3), vec3(-0.5, -6, 3)), Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0))), 5);
		}

		{
			CAGE_TESTCASE("distances (with triangles)");

			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), Triangle(vec3(-1, -1, -2), vec3(-1, -1, 2), vec3(2, 2, 0))), 0);
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 4, 0), vec3(4, -2, 0)), Triangle(vec3(-2, -2, 1), vec3(-2, 4, 1), vec3(4, -2, 1))), 1);
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 4, 0), vec3(4, -2, 0)), Triangle(vec3(0, 0, 0.5), vec3(0, 0, 2), vec3(0, 1, 2))), 0.5);
			test(distance(Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0)), Triangle(vec3(1, 1, -2), vec3(1, 1, 2), vec3(2, 2, 0))), cage::sqrt(2));
			test(distance(Triangle(vec3(1, 1, -2), vec3(1, 1, 2), vec3(2, 2, 0)), Triangle(vec3(-2, -2, 0), vec3(-2, 2, 0), vec3(2, -2, 0))), cage::sqrt(2));
		}

		{
			CAGE_TESTCASE("randomized intersections (with lines)");

			for (uint32 round = 0; round < 10; round++)
			{
				const transform tr = transform(randomDirection3() *randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				const Triangle t = Triangle(randomDirection3() * 10, randomDirection3() * 10, randomDirection3() * 10);
				CAGE_TEST(intersects(l * tr, t * tr) == intersects(l, t));
				CAGE_TEST(intersects(l, t * tr) == intersects(l * inverse(tr), t));
				CAGE_TEST(intersects(l * tr, t) == intersects(l, t * inverse(tr)));
			}
		}

		{
			CAGE_TESTCASE("randomized intersections (with triangles)");

			for (uint32 round = 0; round < 10; round++)
			{
				const transform tr = transform(randomDirection3() *randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);
				const Triangle t1 = Triangle(randomDirection3() * 10, randomDirection3() * 10, randomDirection3() * 10);
				const Triangle t2 = Triangle(randomDirection3() * 10, randomDirection3() * 10, randomDirection3() * 10);
				CAGE_TEST(intersects(t1 * tr, t2 * tr) == intersects(t1, t2));
				CAGE_TEST(intersects(t1, t2 * tr) == intersects(t1 * inverse(tr), t2));
				CAGE_TEST(intersects(t1 * tr, t2) == intersects(t1, t2 * inverse(tr)));
			}
		}
	}

	{
		CAGE_TESTCASE("planes");

		{
			CAGE_TESTCASE("origin & normalized");

			for (int i = 0; i < 5; i++)
			{
				const Plane p(randomChance3() * 100, randomDirection3());
				CAGE_TEST(p.normalized());
				test(distance(p, p.origin()), 0); // origin is a point on the Plane
			}

			CAGE_TEST(!Plane(vec3(), vec3(1, 1, 0)).normalized());
			CAGE_TEST(Plane(vec3(), vec3(1, 1, 0)).normalize().normalized());
		}

		{
			CAGE_TESTCASE("closest point");

			test(closestPoint(Plane(vec3(13, 42, -5), vec3(0, 0, 1)), vec3(123, 456, 1)), vec3(123, 456, -5));
			test(closestPoint(vec3(123, 456, 1), Plane(vec3(13, 42, -5), vec3(0, 0, 1))), vec3(123, 456, -5));
			CAGE_TEST_ASSERTED(closestPoint(Plane(vec3(13, 42, -5), vec3(0, 0, 2)), vec3(123, 456, 1)));
		}

		{
			CAGE_TESTCASE("distances (with points)");

			test(distance(Plane(vec3(13, 42, -5), vec3(0, 0, 1)), vec3(123, 456, 1)), 6);
			test(distance(vec3(123, 456, 1), Plane(vec3(13, 42, -5), vec3(0, 0, 1))), 6);
			test(distance(Plane(vec3(13, 42, -5), vec3(0, 0, -1)), vec3(123, 456, 1)), 6);
			test(distance(Plane(vec3(13, 42, 5), vec3(0, 0, -1)), vec3(123, 456, 1)), 4);
			test(distance(Plane(vec3(13, 42, 5), vec3(0, 1, 0)), vec3(123, 40, 541)), 2);
		}

		{
			CAGE_TESTCASE("distances (with segments)");

			test(distance(Plane(vec3(13, 42, 1), vec3(0, 0, 1)), makeSegment(vec3(123, 456, 7), vec3(890, 123, -4))), 0);
			test(distance(Plane(vec3(13, 42, 1), vec3(0, 0, 1)), makeSegment(vec3(123, 456, 7), vec3(890, 123, 1))), 0);
			test(distance(Plane(vec3(13, 42, -5), vec3(0, 0, 1)), makeSegment(vec3(123, 456, 1), vec3(890, 123, 1))), 6);
			test(distance(makeSegment(vec3(123, 456, 1), vec3(890, 123, 1)), Plane(vec3(13, 42, -5), vec3(0, 0, 1))), 6);
		}

		{
			CAGE_TESTCASE("intersects");

			CAGE_TEST(intersects(Plane(vec3(13, 42, 1), vec3(0, 0, 1)), makeSegment(vec3(123, 456, 7), vec3(890, 123, -4))));
			CAGE_TEST(intersects(makeSegment(vec3(123, 456, 7), vec3(890, 123, -4)), Plane(vec3(13, 42, 1), vec3(0, 0, 1))));
			CAGE_TEST(intersects(Plane(vec3(13, 42, 1), vec3(0, 0, 1)), makeSegment(vec3(123, 456, 7), vec3(890, 123, 1))));
			CAGE_TEST(!intersects(Plane(vec3(13, 42, -5), vec3(0, 0, 1)), makeSegment(vec3(123, 456, 7), vec3(890, 123, 1))));
		}

		{
			CAGE_TESTCASE("intersections");

			test(intersection(Plane(vec3(), vec3(0, 1, 0)), makeLine(vec3(0, 1, 0), vec3(0, -1, 0))), vec3());
			test(intersection(Plane(vec3(), vec3(0, 1, 0)), makeLine(vec3(1, 1, 0), vec3(1, -1, 0))), vec3(1, 0, 0));
			test(intersection(makeLine(vec3(1, 1, 0), vec3(1, -1, 0)), Plane(vec3(), vec3(0, 1, 0))), vec3(1, 0, 0));
			test(intersection(Plane(vec3(13, 5, 42), vec3(0, 1, 0)), makeLine(vec3(1, 1, 0), vec3(1, -1, 0))), vec3(1, 5, 0));
			test(intersection(Plane(vec3(), vec3(0, 1, 0)), makeSegment(vec3(1, 1, 0), vec3(1, -1, 0))), vec3(1, 0, 0));
			CAGE_TEST(!intersection(Plane(vec3(), vec3(0, 1, 0)), makeSegment(vec3(1, 1, 0), vec3(1, 2, 0))).valid());
		}
	}

	{
		CAGE_TESTCASE("spheres");

		{
			CAGE_TESTCASE("distances");

			test(distance(makeLine(vec3(1, 10, 20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(Sphere(vec3(1, 2, 3), 3), makeLine(vec3(1, 10, 20), vec3(1, 10, 25))), 5);
			test(distance(makeLine(vec3(1, 10, -20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeLine(vec3(1, 4, -20), vec3(1, 4, 25)), Sphere(vec3(1, 2, 3), 3)), 0);

			CAGE_TEST(distance(makeRay(vec3(1, 10, 20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeRay(vec3(1, 10, -20), vec3(1, 10, -25)), Sphere(vec3(1, 2, 3), 3)) > 10);
			test(distance(makeRay(vec3(1, 10, -20), vec3(1, 10, -15)), Sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeRay(vec3(1, 4, -20), vec3(1, 4, 25)), Sphere(vec3(1, 2, 3), 3)), 0);

			CAGE_TEST(distance(makeSegment(vec3(1, 10, 20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeSegment(vec3(1, 10, -20), vec3(1, 10, -25)), Sphere(vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeSegment(vec3(1, 10, -20), vec3(1, 10, -15)), Sphere(vec3(1, 2, 3), 3)) > 5);
			test(distance(makeSegment(vec3(1, 10, -20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeSegment(vec3(1, 4, -20), vec3(1, 4, 25)), Sphere(vec3(1, 2, 3), 3)), 0);
		}

		{
			CAGE_TESTCASE("intersects");

			CAGE_TEST(!intersects(makeLine(vec3(1, 10, 20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeLine(vec3(1, 10, -20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeLine(vec3(1, 4, -20), vec3(1, 4, 25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(Sphere(vec3(1, 2, 3), 3), makeLine(vec3(1, 4, -20), vec3(1, 4, 25))));

			CAGE_TEST(!intersects(makeRay(vec3(1, 10, 20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeRay(vec3(1, 10, -20), vec3(1, 10, -25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeRay(vec3(1, 10, -20), vec3(1, 10, -15)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeRay(vec3(1, 4, -20), vec3(1, 4, 25)), Sphere(vec3(1, 2, 3), 3)));

			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, 20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, -20), vec3(1, 10, -25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, -20), vec3(1, 10, -15)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, -20), vec3(1, 10, 25)), Sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeSegment(vec3(1, 4, -20), vec3(1, 4, 25)), Sphere(vec3(1, 2, 3), 3)));
		}

		{
			CAGE_TESTCASE("intersections");
			// todo
		}
	}

	{
		CAGE_TESTCASE("aabb");

		{
			CAGE_TESTCASE("ctors, isEmpty, volume, addition");
			Aabb a;
			CAGE_TEST(a.empty());
			test(a.volume(), 0);
			Aabb b(vec3(1, 5, 3));
			CAGE_TEST(!b.empty());
			test(b.volume(), 0);
			Aabb c(vec3(1, -1, 1), vec3(-1, 1, -1));
			CAGE_TEST(!c.empty());
			test(c.volume(), 8);
			Aabb d = a + b;
			CAGE_TEST(!d.empty());
			test(d.volume(), 0);
			Aabb e = b + a;
			CAGE_TEST(!e.empty());
			test(e.volume(), 0);
			Aabb f = a + c;
			CAGE_TEST(!f.empty());
			test(f.volume(), 8);
			Aabb g = c + a;
			CAGE_TEST(!g.empty());
			test(g.volume(), 8);
			Aabb h = c + b;
			CAGE_TEST(!h.empty());
			test(h.volume(), 48);
			CAGE_TEST(Aabb::Universe().diagonal() > 100);
			CAGE_TEST(Aabb::Universe().diagonal() == real::Infinity());
		}

		{
			CAGE_TESTCASE("construct from plane");
			const Aabb a = Aabb(Plane(vec3(123, 456, 789), vec3(0, 1, 0)));
			testEx(a.a, vec3(-real::Infinity(), 456, -real::Infinity()));
			testEx(a.b, vec3(real::Infinity(), 456, real::Infinity()));
			const Aabb b = Aabb(Plane(vec3(123, 456, 789), vec3(0, 0, -1)));
			testEx(b.a, vec3(-real::Infinity(), -real::Infinity(), 789));
			testEx(b.b, vec3(real::Infinity(), real::Infinity(), 789));
			const Aabb c = Aabb(Plane(vec3(123, 456, 789), normalize(vec3(1, 2, -1))));
			testEx(c.a, vec3(-real::Infinity()));
			testEx(c.b, vec3(real::Infinity()));
		}

		{
			CAGE_TESTCASE("intersects, intersections (with aabb)");
			const Aabb a(vec3(-5, -6, -3), vec3(-4, -4, -1));
			const Aabb b(vec3(1, 3, 4), vec3(4, 7, 8));
			const Aabb c(vec3(-10, -10, -10), vec3());
			const Aabb d(vec3(), vec3(10, 10, 10));
			const Aabb e(vec3(-5, -5, -5), vec3(5, 5, 5));
			CAGE_TEST(!intersects(a, b));
			CAGE_TEST(intersection(a, b).empty());
			CAGE_TEST(intersects(c, d));
			CAGE_TEST(!intersection(c, d).empty());
			test(intersection(c, d), Aabb(vec3()));
			CAGE_TEST(intersects(a, c));
			CAGE_TEST(intersects(b, d));
			test(intersection(a, c), a);
			test(intersection(b, d), b);
			CAGE_TEST(intersects(a, e));
			CAGE_TEST(intersects(b, e));
			CAGE_TEST(intersects(c, e));
			CAGE_TEST(intersects(d, e));
			CAGE_TEST(intersects(e, e));
			test(intersection(a, e), Aabb(vec3(-5, -5, -3), vec3(-4, -4, -1)));
			test(intersection(b, e), Aabb(vec3(1, 3, 4), vec3(4, 5, 5)));
			test(intersection(c, e), Aabb(vec3(-5, -5, -5), vec3()));
			test(intersection(d, e), Aabb(vec3(5, 5, 5), vec3()));
			test(intersection(e, e), e);
			test(distance(c, d), 0);
			test(distance(a, b), distance(a.b, b.a));
		}

		{
			CAGE_TESTCASE("ray test");
			const Aabb a(vec3(-5, -6, -3), vec3(-4, -4, -1));
			CAGE_TEST(intersects(makeSegment(vec3(-4, -4, -10), vec3(-5, -5, 10)), a));
			CAGE_TEST(intersects(a, makeSegment(vec3(-4, -4, -10), vec3(-5, -5, 10))));
			CAGE_TEST(intersects(makeSegment(vec3(-10, -12, -6), vec3(-5, -6, -2)), a));
			CAGE_TEST(!intersects(makeSegment(vec3(-4, -4, -10), vec3(-5, -5, -5)), a));
			CAGE_TEST(!intersects(makeSegment(vec3(-5, -5, -5), vec3(-4, -4, -5)), a));
		}

		{
			CAGE_TESTCASE("distance to point");
			const Aabb a(vec3(1, 3, 4), vec3(4, 7, 8));
			test(distance(a, vec3(0, 0, 0)), length(vec3(1, 3, 4)));
			test(distance(a, vec3(2, 7, 6)), 0);
			test(distance(a, vec3(3, 3, 10)), 2);
			test(distance(vec3(3, 3, 10), a), 2);
		}

		{
			CAGE_TESTCASE("transformation");
			const Aabb a(vec3(-5, -6, -3), vec3(-4, -4, -1));
			const Aabb b(vec3(1, 3, 4), vec3(4, 7, 8));
			const Aabb c(vec3(-10, -10, -10), vec3());
			const Aabb d(vec3(), vec3(10, 10, 10));
			const Aabb e(vec3(-5, -5, -5), vec3(5, 5, 5));
			const mat4 rot1(quat(degs(30), degs(), degs()));
			const mat4 rot2(quat(degs(), degs(315), degs()));
			const mat4 tran(vec3(0, 10, 0));
			const mat4 scl = mat4::scale(3);
			CAGE_TEST((a * rot1).volume() > a.volume());
			CAGE_TEST((a * rot1 * rot2).volume() > a.volume());
			CAGE_TEST((a * scl).volume() > a.volume());
			test((a * tran).volume(), a.volume());
		}

		{
			CAGE_TESTCASE("triangle-aabb intersects");
			{
				const Aabb box(vec3(5, 3, 8), vec3(12, 9, 10));
				CAGE_TEST(intersects(Triangle(vec3(6, 7, 8), vec3(11, 3, 8), vec3(11, 9, 10)), box)); // Triangle fully inside
				CAGE_TEST(intersects(box, Triangle(vec3(6, 7, 8), vec3(11, 3, 8), vec3(11, 9, 10)))); // Triangle fully inside
				CAGE_TEST(!intersects(Triangle(vec3(-6, 7, 8), vec3(-11, 3, 8), vec3(-11, 9, 10)), box)); // Triangle fully outside
			}
			{ // triangles with all vertices outside
				const Aabb box(vec3(0, 0, 0), vec3(2, 2, 2));
				CAGE_TEST(intersects(Triangle(vec3(-1, -1, 1), vec3(3, -1, 1), vec3(1, 3, 1)), box)); // Plane cut
				CAGE_TEST(intersects(Triangle(vec3(0, -1, 1), vec3(3, -1, 1), vec3(3, 2, 1)), box)); // one edge
				CAGE_TEST(intersects(Triangle(vec3(0, 0, 5), vec3(0, 5, 0), vec3(5, 0, 0)), box)); // cut a corner
				CAGE_TEST(intersects(Triangle(vec3(-5, 1, 1), vec3(5, 1, 1), vec3(5, 1, 3)), box)); // needle
			}
		}

		{
			CAGE_TESTCASE("plane-aabb intersects");
			const Aabb box(vec3(-5, -5, -5), vec3(5, 5, 5));
			const Plane a(vec3(1, 2, 3), vec3(0, 0, 1));
			const Plane b(vec3(1, 2, 13), vec3(0, 0, 1));
			const Plane c(vec3(1, 42, 3), vec3(0, 0, 1));
			CAGE_TEST(intersects(box, a));
			CAGE_TEST(!intersects(box, b));
			CAGE_TEST(intersects(box, c));
		}
	}

	{
		CAGE_TESTCASE("frustum");

		{
			CAGE_TESTCASE("convert perspective frustum to box/sphere");
			const mat4 proj = perspectiveProjection(degs(90), 1, 10, 20);
			const Frustum frustum = Frustum(transform(vec3(13, 42, 5), quat(degs(), degs(180), degs())), proj);
			const Aabb box = Aabb(frustum);
			test(box, Aabb(vec3(-7, 22, 15), vec3(33, 62, 25)));
			const Sphere sphere = Sphere(frustum);
			test(sphere, Sphere(vec3(13, 42, 25), sqrt(sqr(20) * 2)));
		}

		{
			CAGE_TESTCASE("convert orthographic frustum to box/sphere");
			const mat4 proj = orthographicProjection(42, 62, 13, 23, 10, 20);
			const Frustum frustum = Frustum(transform(vec3(5)), proj);
			const Aabb box = Aabb(frustum);
			test(box, Aabb(vec3(47, 18, -15), vec3(67, 28, -5)));
			const Sphere sphere = Sphere(frustum);
			test(sphere, Sphere(box)); // in this case (the frustum is axis aligned) the sphere should be the same when constructed from the frustum as from the box
		}

		{
			CAGE_TESTCASE("point-frustum intersections");
			const mat4 proj = perspectiveProjection(degs(90), 1, 10, 20);
			const Frustum frustum = Frustum(transform(vec3(13, 42, 5), quat(degs(), degs(180), degs())), proj);
			CAGE_TEST(intersects(frustum, vec3(5, 40, 20)));
			CAGE_TEST(intersects(vec3(5, 40, 20), frustum));
			CAGE_TEST(!intersects(frustum, vec3(5, 40, 10)));
			CAGE_TEST(!intersects(frustum, vec3(5, 40, 30)));
			CAGE_TEST(!intersects(frustum, vec3(-10, 40, 20)));
			CAGE_TEST(!intersects(frustum, vec3(30, 40, 20)));
			CAGE_TEST(!intersects(frustum, vec3(5, 20, 20)));
			CAGE_TEST(!intersects(frustum, vec3(5, 70, 20)));
		}

		{
			CAGE_TESTCASE("aabb-frustum intersections");
			const Aabb a(vec3(1, 1, -7), vec3(3, 5, -1));
			const mat4 proj = perspectiveProjection(degs(90), 1, 2, 10);
			// frustum moved back and forth
			CAGE_TEST(intersects(a, Frustum(transform(vec3(2, 3, -10)), proj)) == false);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(2, 3, -5)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(2, 3, 0)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(2, 3, 3)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(2, 3, 10)), proj)) == false);
			// frustum moved left and right
			CAGE_TEST(intersects(a, Frustum(transform(vec3(-10, 3, 0)), proj)) == false);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(0, 3, 0)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(5, 3, 0)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(transform(vec3(15, 3, 0)), proj)) == false);
		}
	}

	{
		CAGE_TESTCASE("stringize");
		const string s = detail::StringizerBase<5432>()
			+ makeSegment(vec3(1, 2, 3), vec3(4, 5, 6)) + ", "
			+ Triangle() + ", "
			+ Plane() + ", "
			+ Sphere() + ", "
			+ Aabb() + ", "
			+ Cone() + ", "
			+ Frustum();
	}
}
