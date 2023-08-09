#include <cmath>

#include "main.h"

#include <cage-core/camera.h>
#include <cage-core/geometry.h>
#include <cage-core/math.h>

void test(Real a, Real b);
void test(Rads a, Rads b);
void test(const Quat &a, const Quat &b);
void test(const Vec3 &a, const Vec3 &b);
void test(const Vec4 &a, const Vec4 &b);
void test(const Mat3 &a, const Mat3 &b);
void test(const Mat4 &a, const Mat4 &b);
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
	void testEx(Real a, Real b)
	{
		if (!a.finite() || !b.finite())
		{
			// test without epsilon
			CAGE_TEST(a == b);
		}
		else
			test(a, b);
	}

	void testEx(const Vec3 &a, const Vec3 &b)
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

		CAGE_TEST(intersects(Vec3(1, 2, 3), Vec3(1, 2, 3)));
		CAGE_TEST(!intersects(Vec3(1, 2, 3), Vec3(3, 2, 1)));

		test(distance(Vec3(1, 2, 3), Vec3(1, 2, 3)), 0);
		test(distance(Vec3(1, 2, 3), Vec3(3, 2, 1)), sqrt(8));

		test(distance(Vec2(1, 2), Vec2(1, 2)), 0);
	}

	{
		CAGE_TESTCASE("lines");

		{
			CAGE_TESTCASE("normalization");

			Line l = makeSegment(Vec3(1, 2, 3), Vec3(3, 4, 5));
			CAGE_TEST(!l.isPoint() && l.isSegment() && !l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());
			test(l.direction, normalize(Vec3(3, 4, 5) - Vec3(1, 2, 3)));

			l = makeRay(Vec3(1, 2, 3), Vec3(3, 4, 5));
			CAGE_TEST(!l.isPoint() && !l.isSegment() && l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());
			test(l.direction, normalize(Vec3(3, 4, 5) - Vec3(1, 2, 3)));

			l = makeLine(Vec3(1, 2, 3), Vec3(3, 4, 5));
			CAGE_TEST(!l.isPoint() && !l.isSegment() && !l.isRay() && l.isLine());
			CAGE_TEST(l.normalized());
			test(l.direction, normalize(Vec3(3, 4, 5) - Vec3(1, 2, 3)));

			l = Line(Vec3(1, 2, 3), Vec3(3, 4, 5), -5, 5);
			CAGE_TEST(l.valid());
			CAGE_TEST(!l.normalized());

			l = makeRay(Vec3(1, 2, 3), Vec3(1, 2, 3));
			CAGE_TEST(l.isPoint() && l.isSegment() && !l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());

			l = makeSegment(Vec3(-5, 0, 0), Vec3(5, 0, 0));
			CAGE_TEST(!l.isPoint() && l.isSegment() && !l.isRay() && !l.isLine());
			CAGE_TEST(l.normalized());
			test(l.minimum, 0);
			test(l.maximum, 10);
			test(l.a(), Vec3(-5, 0, 0));
			test(l.b(), Vec3(5, 0, 0));

			l = makeSegment(Vec3(0.1, 0, 16), Vec3(0.1, 0, 12));
			CAGE_TEST(l.normalized());
			test(l.origin, Vec3(0.1, 0, 16));
			test(l.direction, Vec3(0, 0, -1));
			test(l.minimum, 0);
			test(l.maximum, 4);
		}

		{
			CAGE_TESTCASE("distances (segments)");

			const Line a = makeSegment(Vec3(0, -1, 0), Vec3(0, 1, 0));
			const Line b = makeSegment(Vec3(1, -2, 0), Vec3(1, 2, 0));
			const Line c = makeSegment(Vec3(0, 0, 0), Vec3(0, 0, 1));
			const Line d = makeSegment(Vec3(3, -1, 0), Vec3(3, 1, 0));
			test(distance(a, a), 0);
			test(distance(a, b), 1);
			test(distance(b, a), 1);
			test(distance(a, c), 0);
			test(distance(b, c), 1);
			test(distance(a, d), 3);
		}

		{
			CAGE_TESTCASE("distances (lines)");

			test(distance(makeLine(Vec3(), Vec3(0, 0, 1)), makeLine(Vec3(0, 0, 1), Vec3())), 0);
			test(distance(makeLine(Vec3(), Vec3(0, 0, 1)), makeLine(Vec3(), Vec3(0, 1, 0))), 0);
			test(distance(makeLine(Vec3(), Vec3(0, 0, 1)), makeLine(Vec3(1, 0, 1), Vec3(1, 0, 0))), 1);
			test(distance(makeLine(Vec3(), Vec3(0, 0, 1)), makeLine(Vec3(1, 0, 0), Vec3(1, 1, 0))), 1);
		}

		{
			CAGE_TESTCASE("angles");

			const Line a = makeSegment(Vec3(0, -1, 1), Vec3(0, 1, 1));
			const Line b = makeSegment(Vec3(-1, 0, 2), Vec3(1, 0, 2));
			const Line c = makeSegment(Vec3(0, 0, 0), Vec3(1, 1, 1));
			const Line d = makeSegment(Vec3(3, -1, 0), Vec3(3, 1, 0));
			test(angle(a, a), Degs(0));
			test(angle(a, b), Degs(90));
			test(angle(b, a), Degs(90));
			test(angle(a, c), atan2(Real(1), sqrt(2)));
		}

		{
			CAGE_TESTCASE("randomized intersects");

			for (uint32 round = 0; round < 10; round++)
			{
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				CAGE_TEST(intersects(l, l.origin));
				CAGE_TEST(intersects(l, l.origin + l.direction));
				CAGE_TEST(intersects(l, l.origin - l.direction));
				CAGE_TEST(!intersects(l, l.origin + Vec3(1, 0, 0)));
			}
		}

		{
			CAGE_TESTCASE("randomized transformations");

			for (uint32 round = 0; round < 5; round++)
			{
				const Line l = makeLine(randomDirection3() * 10, randomDirection3() * 10);
				const Vec3 p = l.origin + l.direction * (randomChance() + 1);
				CAGE_TEST(intersects(l, p));
				for (uint32 round2 = 0; round2 < 5; round2++)
				{
					const Transform tr = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() + 1);
					const Line lt = l * tr;
					const Vec3 pt = p * tr;
					CAGE_TEST(intersects(lt, pt));
				}
			}
		}
	}

	{
		CAGE_TESTCASE("triangles");

		{
			CAGE_TESTCASE("basics");

			Triangle t1(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0));
			const Triangle t2(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1));
			const Triangle t3(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0));
			CAGE_TEST(!intersects(t1, t2));
			CAGE_TEST(intersects(t1, t3));
			CAGE_TEST(intersects(t2, t3));
			t1.area();
			t1.center();
			t1.normal();
			t1 *= Mat4(Vec3(1, 2, 3));
			t1 *= Mat4(Vec3(4, 5, 10), Quat(Degs(), Degs(42), Degs()), Vec3(3, 2, 1));
		}

		{
			CAGE_TESTCASE("tests");

			const Triangle t1(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0));
			test(t1.normal(), -t1.flip().normal());
			CAGE_TEST(!t1.degenerated());
			CAGE_TEST(Triangle(Vec3(1, 2, 3), Vec3(3, 2, 1), Vec3(1, 2, 3)).degenerated()); // two vertices are the same
			CAGE_TEST(Triangle(Vec3(1, 2, 3), Vec3(2, 4, 6), Vec3(3, 6, 9)).degenerated()); // vertices are collinear
		}

		{
			CAGE_TESTCASE("closest point");

			const Triangle tri = Triangle(Vec3(), Vec3(2, 0, 0), Vec3(0, 2, 0));
			test(closestPoint(tri, Vec3()), Vec3());
			test(closestPoint(tri, Vec3(2, 0, 0)), Vec3(2, 0, 0));
			test(closestPoint(tri, Vec3(0, 2, 0)), Vec3(0, 2, 0));
			test(closestPoint(tri, Vec3(1, 1, 0)), Vec3(1, 1, 0));
			test(closestPoint(tri, Vec3(2, 2, 0)), Vec3(1, 1, 0));
			test(closestPoint(tri, Vec3(1.5, 1.5, 0)), Vec3(1, 1, 0));
			test(closestPoint(tri, Vec3(0.5, 0.5, 0)), Vec3(0.5, 0.5, 0));
			test(closestPoint(tri, Vec3(1, 0, 0)), Vec3(1, 0, 0));
			test(closestPoint(tri, Vec3(0, 1, 0)), Vec3(0, 1, 0));
			test(closestPoint(tri, Vec3(5, 0, 0)), Vec3(2, 0, 0));
			test(closestPoint(tri, Vec3(0, 5, 0)), Vec3(0, 2, 0));
			test(closestPoint(tri, Vec3(-5, 0, 0)), Vec3());
			test(closestPoint(tri, Vec3(0, -5, 0)), Vec3());
			test(closestPoint(tri, Vec3(-5, -5, 0)), Vec3());
			test(closestPoint(tri, Vec3(5, -5, 0)), Vec3(2, 0, 0));
			test(closestPoint(tri, Vec3(-5, 5, 0)), Vec3(0, 2, 0));
			test(closestPoint(tri, Vec3(1, -5, 0)), Vec3(1, 0, 0));
			test(closestPoint(tri, Vec3(-5, 1, 0)), Vec3(0, 1, 0));
			test(closestPoint(Vec3(-5, 1, 0), tri), Vec3(0, 1, 0));
		}

		{
			CAGE_TESTCASE("randomized closest point");

			for (uint32 round = 0; round < 10; round++)
			{
				const Transform tr = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat());
				const Triangle tri = Triangle(Vec3(), Vec3(2, 0, 0), Vec3(0, 2, 0)) * tr;
				const Real z = randomRange(-100, 100);
				test(closestPoint(tri, Vec3() * tr), Vec3() * tr);
				test(closestPoint(tri, Vec3(2, 0, z) * tr), Vec3(2, 0, 0) * tr);
				test(closestPoint(tri, Vec3(0, 2, z) * tr), Vec3(0, 2, 0) * tr);
				test(closestPoint(tri, Vec3(1, 1, z) * tr), Vec3(1, 1, 0) * tr);
				test(closestPoint(tri, Vec3(2, 2, z) * tr), Vec3(1, 1, 0) * tr);
				test(closestPoint(tri, Vec3(1.5, 1.5, z) * tr), Vec3(1, 1, 0) * tr);
				test(closestPoint(tri, Vec3(0.5, 0.5, z) * tr), Vec3(0.5, 0.5, 0) * tr);
				test(closestPoint(tri, Vec3(1, 0, z) * tr), Vec3(1, 0, 0) * tr);
				test(closestPoint(tri, Vec3(0, 1, z) * tr), Vec3(0, 1, 0) * tr);
				test(closestPoint(tri, Vec3(5, 0, z) * tr), Vec3(2, 0, 0) * tr);
				test(closestPoint(tri, Vec3(0, 5, z) * tr), Vec3(0, 2, 0) * tr);
				test(closestPoint(tri, Vec3(-5, 0, z) * tr), Vec3() * tr);
				test(closestPoint(tri, Vec3(0, -5, z) * tr), Vec3() * tr);
				test(closestPoint(tri, Vec3(-5, -5, z) * tr), Vec3() * tr);
				test(closestPoint(tri, Vec3(5, -5, z) * tr), Vec3(2, 0, 0) * tr);
				test(closestPoint(tri, Vec3(-5, 5, z) * tr), Vec3(0, 2, 0) * tr);
				test(closestPoint(tri, Vec3(1, -5, z) * tr), Vec3(1, 0, 0) * tr);
				test(closestPoint(tri, Vec3(-5, 1, z) * tr), Vec3(0, 1, 0) * tr);
			}
		}

		{
			CAGE_TESTCASE("distances (with lines)");

			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), makeSegment(Vec3(-1, -1, -2), Vec3(-1, -1, 2))), 0);
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), makeSegment(Vec3(1, 1, -2), Vec3(1, 1, 2))), cage::sqrt(2));
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), makeSegment(Vec3(-5, 0, 1), Vec3(5, 0, 1))), 1);
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), makeSegment(Vec3(-1.5, 0, 1), Vec3(-0.5, 0, 1))), 1);
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), makeSegment(Vec3(-1.5, -6, 3), Vec3(-0.5, -6, 3))), 5);
			test(distance(makeSegment(Vec3(-1.5, -6, 3), Vec3(-0.5, -6, 3)), Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0))), 5);
		}

		{
			CAGE_TESTCASE("distances (with triangles)");

			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), Triangle(Vec3(-1, -1, -2), Vec3(-1, -1, 2), Vec3(2, 2, 0))), 0);
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 4, 0), Vec3(4, -2, 0)), Triangle(Vec3(-2, -2, 1), Vec3(-2, 4, 1), Vec3(4, -2, 1))), 1);
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 4, 0), Vec3(4, -2, 0)), Triangle(Vec3(0, 0, 0.5), Vec3(0, 0, 2), Vec3(0, 1, 2))), 0.5);
			test(distance(Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0)), Triangle(Vec3(1, 1, -2), Vec3(1, 1, 2), Vec3(2, 2, 0))), cage::sqrt(2));
			test(distance(Triangle(Vec3(1, 1, -2), Vec3(1, 1, 2), Vec3(2, 2, 0)), Triangle(Vec3(-2, -2, 0), Vec3(-2, 2, 0), Vec3(2, -2, 0))), cage::sqrt(2));
		}

		{
			CAGE_TESTCASE("intersections (with lines)");

			const Triangle t1(Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, 2, 0));
			const Triangle t2(Vec3(-2, 0, 1), Vec3(2, 0, 1), Vec3(0, 3, 1));
			const Triangle t3(Vec3(-2, 1, -5), Vec3(0, 1, 5), Vec3(2, 1, 0));
			test(intersection(makeSegment(Vec3(0, 1, -1), Vec3(0, 1, 1)), t1), Vec3(0, 1, 0));
			test(intersection(t1, makeSegment(Vec3(0, 1, -1), Vec3(0, 1, 1))), Vec3(0, 1, 0));
			CAGE_TEST(intersects(makeSegment(Vec3(0, 1, -1), Vec3(0, 1, 1)), t1));
			CAGE_TEST(intersects(t1, makeSegment(Vec3(0, 1, -1), Vec3(0, 1, 1))));
			CAGE_TEST(!intersection(makeSegment(Vec3(5, 1, -1), Vec3(0, 1, 1)), t1).valid());
			CAGE_TEST(!intersects(makeSegment(Vec3(5, 1, -1), Vec3(0, 1, 1)), t1));
			test(intersection(makeSegment(Vec3(2, 5, 0), Vec3(2, -5, 0)), t3), Vec3(2, 1, 0));
			CAGE_TEST(intersects(makeSegment(Vec3(2, 5, 0), Vec3(2, -5, 0)), t3));
			CAGE_TEST(!intersection(makeSegment(Vec3(2, 0, 0), Vec3(2, -5, 0)), t3).valid());
			CAGE_TEST(!intersects(makeSegment(Vec3(2, 0, 0), Vec3(2, -5, 0)), t3));
		}

		{
			CAGE_TESTCASE("intersects (with triangles)");

			const Triangle a = Triangle(Vec3(10.5860138, -0.126804054, -10.5860195), Vec3(15.8790216, -0.190205932, -15.8790274), Vec3(15.8786421, -0.253607780, -15.8786469));
			const Triangle b = Triangle(Vec3(10.5860157, -0.126804069, -10.5860205), Vec3(5.29338837, -3.57627869e-07, -5.29339361), Vec3(5.29300880, -0.0634022281, -5.29301405));
			CAGE_TEST(intersects(a, b) == intersects(b, a));

			//const Triangle c = Triangle(Vec3(5.23147297, 5.81014061, -6.23489857), Vec3(7.14238548, 3.17999482, -6.23489857), Vec3(3.96372533, 1.76476419, -9.00968838));
			//const Triangle d = Triangle(Vec3(-4.37113897e-07, -0.00000000, -10.0000000), Vec3(3.96372533, 1.76476419, -9.00968838), Vec3(4.33883762, 0.00000000, -9.00968838));
			//CAGE_TEST(intersects(c, d));
			//CAGE_TEST(intersects(d, c));
		}

		{
			CAGE_TESTCASE("randomized intersections (with lines)");

			for (uint32 round = 0; round < 10; round++)
			{
				const Transform tr = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);
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
				const Transform tr = Transform(randomDirection3() * randomChance() * 10, randomDirectionQuat(), randomChance() * 10 + 1);
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

			CAGE_TEST(!Plane(Vec3(), Vec3(1, 1, 0)).normalized());
			CAGE_TEST(Plane(Vec3(), Vec3(1, 1, 0)).normalize().normalized());
		}

		{
			CAGE_TESTCASE("closest point");

			test(closestPoint(Plane(Vec3(13, 42, -5), Vec3(0, 0, 1)), Vec3(123, 456, 1)), Vec3(123, 456, -5));
			test(closestPoint(Vec3(123, 456, 1), Plane(Vec3(13, 42, -5), Vec3(0, 0, 1))), Vec3(123, 456, -5));
			CAGE_TEST_ASSERTED(closestPoint(Plane(Vec3(13, 42, -5), Vec3(0, 0, 2)), Vec3(123, 456, 1)));
		}

		{
			CAGE_TESTCASE("distances (with points)");

			test(distance(Plane(Vec3(13, 42, -5), Vec3(0, 0, 1)), Vec3(123, 456, 1)), 6);
			test(distance(Vec3(123, 456, 1), Plane(Vec3(13, 42, -5), Vec3(0, 0, 1))), 6);
			test(distance(Plane(Vec3(13, 42, -5), Vec3(0, 0, -1)), Vec3(123, 456, 1)), 6);
			test(distance(Plane(Vec3(13, 42, 5), Vec3(0, 0, -1)), Vec3(123, 456, 1)), 4);
			test(distance(Plane(Vec3(13, 42, 5), Vec3(0, 1, 0)), Vec3(123, 40, 541)), 2);
		}

		{
			CAGE_TESTCASE("distances (with segments)");

			test(distance(Plane(Vec3(13, 42, 1), Vec3(0, 0, 1)), makeSegment(Vec3(123, 456, 7), Vec3(890, 123, -4))), 0);
			test(distance(Plane(Vec3(13, 42, 1), Vec3(0, 0, 1)), makeSegment(Vec3(123, 456, 7), Vec3(890, 123, 1))), 0);
			test(distance(Plane(Vec3(13, 42, -5), Vec3(0, 0, 1)), makeSegment(Vec3(123, 456, 1), Vec3(890, 123, 1))), 6);
			test(distance(makeSegment(Vec3(123, 456, 1), Vec3(890, 123, 1)), Plane(Vec3(13, 42, -5), Vec3(0, 0, 1))), 6);
		}

		{
			CAGE_TESTCASE("intersects");

			CAGE_TEST(intersects(Plane(Vec3(13, 42, 1), Vec3(0, 0, 1)), makeSegment(Vec3(123, 456, 7), Vec3(890, 123, -4))));
			CAGE_TEST(intersects(makeSegment(Vec3(123, 456, 7), Vec3(890, 123, -4)), Plane(Vec3(13, 42, 1), Vec3(0, 0, 1))));
			CAGE_TEST(intersects(Plane(Vec3(13, 42, 1), Vec3(0, 0, 1)), makeSegment(Vec3(123, 456, 7), Vec3(890, 123, 1))));
			CAGE_TEST(!intersects(Plane(Vec3(13, 42, -5), Vec3(0, 0, 1)), makeSegment(Vec3(123, 456, 7), Vec3(890, 123, 1))));
		}

		{
			CAGE_TESTCASE("intersections");

			test(intersection(Plane(Vec3(), Vec3(0, 1, 0)), makeLine(Vec3(0, 1, 0), Vec3(0, -1, 0))), Vec3());
			test(intersection(Plane(Vec3(), Vec3(0, 1, 0)), makeLine(Vec3(1, 1, 0), Vec3(1, -1, 0))), Vec3(1, 0, 0));
			test(intersection(makeLine(Vec3(1, 1, 0), Vec3(1, -1, 0)), Plane(Vec3(), Vec3(0, 1, 0))), Vec3(1, 0, 0));
			test(intersection(Plane(Vec3(13, 5, 42), Vec3(0, 1, 0)), makeLine(Vec3(1, 1, 0), Vec3(1, -1, 0))), Vec3(1, 5, 0));
			test(intersection(Plane(Vec3(), Vec3(0, 1, 0)), makeSegment(Vec3(1, 1, 0), Vec3(1, -1, 0))), Vec3(1, 0, 0));
			CAGE_TEST(!intersection(Plane(Vec3(), Vec3(0, 1, 0)), makeSegment(Vec3(1, 1, 0), Vec3(1, 2, 0))).valid());
		}
	}

	{
		CAGE_TESTCASE("spheres");

		{
			CAGE_TESTCASE("distances");

			test(distance(makeLine(Vec3(1, 10, 20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)), 5);
			test(distance(Sphere(Vec3(1, 2, 3), 3), makeLine(Vec3(1, 10, 20), Vec3(1, 10, 25))), 5);
			test(distance(makeLine(Vec3(1, 10, -20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)), 5);
			test(distance(makeLine(Vec3(1, 4, -20), Vec3(1, 4, 25)), Sphere(Vec3(1, 2, 3), 3)), 0);

			CAGE_TEST(distance(makeRay(Vec3(1, 10, 20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeRay(Vec3(1, 10, -20), Vec3(1, 10, -25)), Sphere(Vec3(1, 2, 3), 3)) > 10);
			test(distance(makeRay(Vec3(1, 10, -20), Vec3(1, 10, -15)), Sphere(Vec3(1, 2, 3), 3)), 5);
			test(distance(makeRay(Vec3(1, 4, -20), Vec3(1, 4, 25)), Sphere(Vec3(1, 2, 3), 3)), 0);

			CAGE_TEST(distance(makeSegment(Vec3(1, 10, 20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeSegment(Vec3(1, 10, -20), Vec3(1, 10, -25)), Sphere(Vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeSegment(Vec3(1, 10, -20), Vec3(1, 10, -15)), Sphere(Vec3(1, 2, 3), 3)) > 5);
			test(distance(makeSegment(Vec3(1, 10, -20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)), 5);
			test(distance(makeSegment(Vec3(1, 4, -20), Vec3(1, 4, 25)), Sphere(Vec3(1, 2, 3), 3)), 0);
		}

		{
			CAGE_TESTCASE("intersects");

			CAGE_TEST(!intersects(makeLine(Vec3(1, 10, 20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeLine(Vec3(1, 10, -20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeLine(Vec3(1, 4, -20), Vec3(1, 4, 25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(Sphere(Vec3(1, 2, 3), 3), makeLine(Vec3(1, 4, -20), Vec3(1, 4, 25))));

			CAGE_TEST(!intersects(makeRay(Vec3(1, 10, 20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeRay(Vec3(1, 10, -20), Vec3(1, 10, -25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeRay(Vec3(1, 10, -20), Vec3(1, 10, -15)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeRay(Vec3(1, 4, -20), Vec3(1, 4, 25)), Sphere(Vec3(1, 2, 3), 3)));

			CAGE_TEST(!intersects(makeSegment(Vec3(1, 10, 20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(Vec3(1, 10, -20), Vec3(1, 10, -25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(Vec3(1, 10, -20), Vec3(1, 10, -15)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(Vec3(1, 10, -20), Vec3(1, 10, 25)), Sphere(Vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeSegment(Vec3(1, 4, -20), Vec3(1, 4, 25)), Sphere(Vec3(1, 2, 3), 3)));
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
			Aabb b(Vec3(1, 5, 3));
			CAGE_TEST(!b.empty());
			test(b.volume(), 0);
			Aabb c(Vec3(1, -1, 1), Vec3(-1, 1, -1));
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
			CAGE_TEST(Aabb::Universe().diagonal() == Real::Infinity());
		}

		{
			CAGE_TESTCASE("construct from plane");
			const Aabb a = Aabb(Plane(Vec3(123, 456, 789), Vec3(0, 1, 0)));
			testEx(a.a, Vec3(-Real::Infinity(), 456, -Real::Infinity()));
			testEx(a.b, Vec3(Real::Infinity(), 456, Real::Infinity()));
			const Aabb b = Aabb(Plane(Vec3(123, 456, 789), Vec3(0, 0, -1)));
			testEx(b.a, Vec3(-Real::Infinity(), -Real::Infinity(), 789));
			testEx(b.b, Vec3(Real::Infinity(), Real::Infinity(), 789));
			const Aabb c = Aabb(Plane(Vec3(123, 456, 789), normalize(Vec3(1, 2, -1))));
			testEx(c.a, Vec3(-Real::Infinity()));
			testEx(c.b, Vec3(Real::Infinity()));
		}

		{
			CAGE_TESTCASE("intersects, intersections (with aabb)");
			const Aabb a(Vec3(-5, -6, -3), Vec3(-4, -4, -1));
			const Aabb b(Vec3(1, 3, 4), Vec3(4, 7, 8));
			const Aabb c(Vec3(-10, -10, -10), Vec3());
			const Aabb d(Vec3(), Vec3(10, 10, 10));
			const Aabb e(Vec3(-5, -5, -5), Vec3(5, 5, 5));
			CAGE_TEST(!intersects(a, b));
			CAGE_TEST(intersection(a, b).empty());
			CAGE_TEST(intersects(c, d));
			CAGE_TEST(!intersection(c, d).empty());
			test(intersection(c, d), Aabb(Vec3()));
			CAGE_TEST(intersects(a, c));
			CAGE_TEST(intersects(b, d));
			test(intersection(a, c), a);
			test(intersection(b, d), b);
			CAGE_TEST(intersects(a, e));
			CAGE_TEST(intersects(b, e));
			CAGE_TEST(intersects(c, e));
			CAGE_TEST(intersects(d, e));
			CAGE_TEST(intersects(e, e));
			test(intersection(a, e), Aabb(Vec3(-5, -5, -3), Vec3(-4, -4, -1)));
			test(intersection(b, e), Aabb(Vec3(1, 3, 4), Vec3(4, 5, 5)));
			test(intersection(c, e), Aabb(Vec3(-5, -5, -5), Vec3()));
			test(intersection(d, e), Aabb(Vec3(5, 5, 5), Vec3()));
			test(intersection(e, e), e);
			test(distance(c, d), 0);
			test(distance(a, b), distance(a.b, b.a));
		}

		{
			CAGE_TESTCASE("ray test");
			const Aabb a(Vec3(-5, -6, -3), Vec3(-4, -4, -1));
			CAGE_TEST(intersects(makeSegment(Vec3(-4, -4, -10), Vec3(-5, -5, 10)), a));
			CAGE_TEST(intersects(a, makeSegment(Vec3(-4, -4, -10), Vec3(-5, -5, 10))));
			CAGE_TEST(intersects(makeSegment(Vec3(-10, -12, -6), Vec3(-5, -6, -2)), a));
			CAGE_TEST(!intersects(makeSegment(Vec3(-4, -4, -10), Vec3(-5, -5, -5)), a));
			CAGE_TEST(!intersects(makeSegment(Vec3(-5, -5, -5), Vec3(-4, -4, -5)), a));
		}

		{
			CAGE_TESTCASE("distance to point");
			const Aabb a(Vec3(1, 3, 4), Vec3(4, 7, 8));
			test(distance(a, Vec3(0, 0, 0)), length(Vec3(1, 3, 4)));
			test(distance(a, Vec3(2, 7, 6)), 0);
			test(distance(a, Vec3(3, 3, 10)), 2);
			test(distance(Vec3(3, 3, 10), a), 2);
		}

		{
			CAGE_TESTCASE("transformation");
			const Aabb a(Vec3(-5, -6, -3), Vec3(-4, -4, -1));
			const Aabb b(Vec3(1, 3, 4), Vec3(4, 7, 8));
			const Aabb c(Vec3(-10, -10, -10), Vec3());
			const Aabb d(Vec3(), Vec3(10, 10, 10));
			const Aabb e(Vec3(-5, -5, -5), Vec3(5, 5, 5));
			const Mat4 rot1(Quat(Degs(30), Degs(), Degs()));
			const Mat4 rot2(Quat(Degs(), Degs(315), Degs()));
			const Mat4 tran(Vec3(0, 10, 0));
			const Mat4 scl = Mat4::scale(3);
			CAGE_TEST((a * rot1).volume() > a.volume());
			CAGE_TEST((a * rot1 * rot2).volume() > a.volume());
			CAGE_TEST((a * scl).volume() > a.volume());
			test((a * tran).volume(), a.volume());
		}

		{
			CAGE_TESTCASE("triangle-aabb intersects");
			{
				const Aabb box(Vec3(5, 3, 8), Vec3(12, 9, 10));
				CAGE_TEST(intersects(Triangle(Vec3(6, 7, 8), Vec3(11, 3, 8), Vec3(11, 9, 10)), box)); // Triangle fully inside
				CAGE_TEST(intersects(box, Triangle(Vec3(6, 7, 8), Vec3(11, 3, 8), Vec3(11, 9, 10)))); // Triangle fully inside
				CAGE_TEST(!intersects(Triangle(Vec3(-6, 7, 8), Vec3(-11, 3, 8), Vec3(-11, 9, 10)), box)); // Triangle fully outside
			}
			{ // triangles with all vertices outside
				const Aabb box(Vec3(0, 0, 0), Vec3(2, 2, 2));
				CAGE_TEST(intersects(Triangle(Vec3(-1, -1, 1), Vec3(3, -1, 1), Vec3(1, 3, 1)), box)); // Plane cut
				CAGE_TEST(intersects(Triangle(Vec3(0, -1, 1), Vec3(3, -1, 1), Vec3(3, 2, 1)), box)); // one edge
				CAGE_TEST(intersects(Triangle(Vec3(0, 0, 5), Vec3(0, 5, 0), Vec3(5, 0, 0)), box)); // cut a corner
				CAGE_TEST(intersects(Triangle(Vec3(-5, 1, 1), Vec3(5, 1, 1), Vec3(5, 1, 3)), box)); // needle
			}
		}

		{
			CAGE_TESTCASE("plane-aabb intersects");
			const Aabb box(Vec3(-5, -5, -5), Vec3(5, 5, 5));
			const Plane a(Vec3(1, 2, 3), Vec3(0, 0, 1));
			const Plane b(Vec3(1, 2, 13), Vec3(0, 0, 1));
			const Plane c(Vec3(1, 42, 3), Vec3(0, 0, 1));
			CAGE_TEST(intersects(box, a));
			CAGE_TEST(!intersects(box, b));
			CAGE_TEST(intersects(box, c));
		}
	}

	{
		CAGE_TESTCASE("frustum");

		{
			CAGE_TESTCASE("convert perspective frustum to box/sphere");
			const Mat4 proj = perspectiveProjection(Degs(90), 1, 10, 20);
			const Frustum frustum = Frustum(Transform(Vec3(13, 42, 5), Quat(Degs(), Degs(180), Degs())), proj);
			const Aabb box = Aabb(frustum);
			test(box, Aabb(Vec3(-7, 22, 15), Vec3(33, 62, 25)));
			const Sphere sphere = Sphere(frustum);
			test(sphere, Sphere(Vec3(13, 42, 25), sqrt(sqr(20) * 2)));
		}

		{
			CAGE_TESTCASE("convert orthographic frustum to box/sphere");
			const Mat4 proj = orthographicProjection(42, 62, 13, 23, 10, 20);
			const Frustum frustum = Frustum(Transform(Vec3(5)), proj);
			const Aabb box = Aabb(frustum);
			test(box, Aabb(Vec3(47, 18, -15), Vec3(67, 28, -5)));
			const Sphere sphere = Sphere(frustum);
			test(sphere, Sphere(box)); // in this case (the frustum is axis aligned) the sphere should be the same when constructed from the frustum as from the box
		}

		{
			CAGE_TESTCASE("point-frustum intersections");
			const Mat4 proj = perspectiveProjection(Degs(90), 1, 10, 20);
			const Frustum frustum = Frustum(Transform(Vec3(13, 42, 5), Quat(Degs(), Degs(180), Degs())), proj);
			CAGE_TEST(intersects(frustum, Vec3(5, 40, 20)));
			CAGE_TEST(intersects(Vec3(5, 40, 20), frustum));
			CAGE_TEST(!intersects(frustum, Vec3(5, 40, 10)));
			CAGE_TEST(!intersects(frustum, Vec3(5, 40, 30)));
			CAGE_TEST(!intersects(frustum, Vec3(-10, 40, 20)));
			CAGE_TEST(!intersects(frustum, Vec3(30, 40, 20)));
			CAGE_TEST(!intersects(frustum, Vec3(5, 20, 20)));
			CAGE_TEST(!intersects(frustum, Vec3(5, 70, 20)));
		}

		{
			CAGE_TESTCASE("aabb-frustum intersections");
			const Aabb a(Vec3(1, 1, -7), Vec3(3, 5, -1));
			const Mat4 proj = perspectiveProjection(Degs(90), 1, 2, 10);
			// frustum moved back and forth
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(2, 3, -10)), proj)) == false);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(2, 3, -5)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(2, 3, 0)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(2, 3, 3)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(2, 3, 10)), proj)) == false);
			// frustum moved left and right
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(-10, 3, 0)), proj)) == false);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(0, 3, 0)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(5, 3, 0)), proj)) == true);
			CAGE_TEST(intersects(a, Frustum(Transform(Vec3(15, 3, 0)), proj)) == false);
		}
	}

	{
		CAGE_TESTCASE("stringize");
		const String s = detail::StringizerBase<5432>() + makeSegment(Vec3(1, 2, 3), Vec3(4, 5, 6)) + ", " + Triangle() + ", " + Plane() + ", " + Sphere() + ", " + Aabb() + ", " + Cone() + ", " + Frustum();
	}

	{
		CAGE_TESTCASE("angle quat");
		test(angle(Quat(), Quat()), Degs());
		test(angle(Quat(), Quat(Vec3(1, 0, 0), Vec3(0, 1, 0))), Degs(90));
	}
}
