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
void test(const aabb &a, const aabb &b)
{
	test(a.a, b.a);
	test(a.b, b.b);
}

void testGeometry()
{
	CAGE_TESTCASE("geometry");

	{
		CAGE_TESTCASE("lines");

		{
			CAGE_TESTCASE("normalization");

			line l = makeSegment(vec3(1, 2, 3), vec3(3, 4, 5));
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

			l = line(vec3(1, 2, 3), vec3(3, 4, 5), -5, 5);
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
			CAGE_TESTCASE("distances");
			line a = makeSegment(vec3(0, -1, 0), vec3(0, 1, 0));
			line b = makeSegment(vec3(1, -2, 0), vec3(1, 2, 0));
			line c = makeSegment(vec3(0, 0, 0), vec3(0, 0, 1));
			line d = makeSegment(vec3(3, -1, 0), vec3(3, 1, 0));
			test(distance(a, a), 0);
			test(distance(a, b), 1);
			test(distance(a, c), 0);
			test(distance(b, c), 1);
			test(distance(a, d), 3);
		}

		{
			CAGE_TESTCASE("angles");
			line a = makeSegment(vec3(0, -1, 1), vec3(0, 1, 1));
			line b = makeSegment(vec3(-1, 0, 2), vec3(1, 0, 2));
			line c = makeSegment(vec3(0, 0, 0), vec3(1, 1, 1));
			line d = makeSegment(vec3(3, -1, 0), vec3(3, 1, 0));
			test(angle(a, a), degs(0));
			test(angle(a, b), degs(90));
			test(angle(a, c), atan2(real(1), sqrt(2)));
			CAGE_TEST(!perpendicular(a, a));
			CAGE_TEST(perpendicular(a, b));
			CAGE_TEST(!perpendicular(a, c));
			CAGE_TEST(parallel(a, a));
			CAGE_TEST(!parallel(a, b));
			CAGE_TEST(!parallel(a, c));
			CAGE_TEST(parallel(a, d));
		}

		{
			CAGE_TESTCASE("intersects");
			// todo
		}
	}

	{
		CAGE_TESTCASE("triangles");

		{
			CAGE_TESTCASE("basics");

			triangle t1(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0));
			triangle t2(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1));
			triangle t3(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0));
			CAGE_TEST(parallel(t1, t2));
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
			CAGE_TESTCASE("intersections with lines");

			triangle t1(vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, 2, 0));
			triangle t2(vec3(-2, 0, 1), vec3(2, 0, 1), vec3(0, 3, 1));
			triangle t3(vec3(-2, 1, -5), vec3(0, 1, 5), vec3(2, 1, 0));
			test(intersection(makeSegment(vec3(0, 1, -1), vec3(0, 1, 1)), t1), vec3(0, 1, 0));
			CAGE_TEST(intersects(makeSegment(vec3(0, 1, -1), vec3(0, 1, 1)), t1));
			CAGE_TEST(!intersection(makeSegment(vec3(5, 1, -1), vec3(0, 1, 1)), t1).valid());
			CAGE_TEST(!intersects(makeSegment(vec3(5, 1, -1), vec3(0, 1, 1)), t1));
			test(intersection(makeSegment(vec3(2, 5, 0), vec3(2, -5, 0)), t3), vec3(2, 1, 0));
			CAGE_TEST(intersects(makeSegment(vec3(2, 5, 0), vec3(2, -5, 0)), t3));
			CAGE_TEST(!intersection(makeSegment(vec3(2, 0, 0), vec3(2, -5, 0)), t3).valid());
			CAGE_TEST(!intersects(makeSegment(vec3(2, 0, 0), vec3(2, -5, 0)), t3));
		}
	}

	{
		CAGE_TESTCASE("spheres");

		{
			CAGE_TESTCASE("distances");

			/*
			test(distance(makeLine(vec3(1, 10, 20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeLine(vec3(1, 10, -20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeLine(vec3(1, 4, -20), vec3(1, 4, 25)), sphere(vec3(1, 2, 3), 3)), 0);

			CAGE_TEST(distance(makeRay(vec3(1, 10, 20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeRay(vec3(1, 10, -20), vec3(1, 10, -25)), sphere(vec3(1, 2, 3), 3)) > 10);
			test(distance(makeRay(vec3(1, 10, -20), vec3(1, 10, -15)), sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeRay(vec3(1, 4, -20), vec3(1, 4, 25)), sphere(vec3(1, 2, 3), 3)), 0);

			CAGE_TEST(distance(makeSegment(vec3(1, 10, 20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeSegment(vec3(1, 10, -20), vec3(1, 10, -25)), sphere(vec3(1, 2, 3), 3)) > 10);
			CAGE_TEST(distance(makeSegment(vec3(1, 10, -20), vec3(1, 10, -15)), sphere(vec3(1, 2, 3), 3)) > 5);
			test(distance(makeSegment(vec3(1, 10, -20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)), 5);
			test(distance(makeSegment(vec3(1, 4, -20), vec3(1, 4, 25)), sphere(vec3(1, 2, 3), 3)), 0);
			*/
		}

		{
			CAGE_TESTCASE("intersects");

			/*
			CAGE_TEST(!intersects(makeLine(vec3(1, 10, 20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeLine(vec3(1, 10, -20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeLine(vec3(1, 4, -20), vec3(1, 4, 25)), sphere(vec3(1, 2, 3), 3)));

			CAGE_TEST(!intersects(makeRay(vec3(1, 10, 20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeRay(vec3(1, 10, -20), vec3(1, 10, -25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeRay(vec3(1, 10, -20), vec3(1, 10, -15)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeRay(vec3(1, 4, -20), vec3(1, 4, 25)), sphere(vec3(1, 2, 3), 3)));

			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, 20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, -20), vec3(1, 10, -25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, -20), vec3(1, 10, -15)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(!intersects(makeSegment(vec3(1, 10, -20), vec3(1, 10, 25)), sphere(vec3(1, 2, 3), 3)));
			CAGE_TEST(intersects(makeSegment(vec3(1, 4, -20), vec3(1, 4, 25)), sphere(vec3(1, 2, 3), 3)));
			*/
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
			aabb a;
			CAGE_TEST(a.empty());
			test(a.volume(), 0);
			aabb b(vec3(1, 5, 3));
			CAGE_TEST(!b.empty());
			test(b.volume(), 0);
			aabb c(vec3(1, -1, 1), vec3(-1, 1, -1));
			CAGE_TEST(!c.empty());
			test(c.volume(), 8);
			aabb d = a + b;
			CAGE_TEST(!d.empty());
			test(d.volume(), 0);
			aabb e = b + a;
			CAGE_TEST(!e.empty());
			test(e.volume(), 0);
			aabb f = a + c;
			CAGE_TEST(!f.empty());
			test(f.volume(), 8);
			aabb g = c + a;
			CAGE_TEST(!g.empty());
			test(g.volume(), 8);
			aabb h = c + b;
			CAGE_TEST(!h.empty());
			test(h.volume(), 48);
CAGE_TEST(aabb::Universe().diagonal() > 100);
CAGE_TEST(aabb::Universe().diagonal() == real::Infinity());
		}

		{
		CAGE_TESTCASE("intersects, intersections (with aabb)");
		aabb a(vec3(-5, -6, -3), vec3(-4, -4, -1));
		aabb b(vec3(1, 3, 4), vec3(4, 7, 8));
		aabb c(vec3(-10, -10, -10), vec3());
		aabb d(vec3(), vec3(10, 10, 10));
		aabb e(vec3(-5, -5, -5), vec3(5, 5, 5));
		CAGE_TEST(!intersects(a, b));
		CAGE_TEST(intersection(a, b).empty());
		CAGE_TEST(intersects(c, d));
		CAGE_TEST(!intersection(c, d).empty());
		test(intersection(c, d), aabb(vec3()));
		CAGE_TEST(intersects(a, c));
		CAGE_TEST(intersects(b, d));
		test(intersection(a, c), a);
		test(intersection(b, d), b);
		CAGE_TEST(intersects(a, e));
		CAGE_TEST(intersects(b, e));
		CAGE_TEST(intersects(c, e));
		CAGE_TEST(intersects(d, e));
		CAGE_TEST(intersects(e, e));
		test(intersection(a, e), aabb(vec3(-5, -5, -3), vec3(-4, -4, -1)));
		test(intersection(b, e), aabb(vec3(1, 3, 4), vec3(4, 5, 5)));
		test(intersection(c, e), aabb(vec3(-5, -5, -5), vec3()));
		test(intersection(d, e), aabb(vec3(5, 5, 5), vec3()));
		test(intersection(e, e), e);
		}

		{
			CAGE_TESTCASE("ray test");
			aabb a(vec3(-5, -6, -3), vec3(-4, -4, -1));
			CAGE_TEST(intersects(makeSegment(vec3(-4, -4, -10), vec3(-5, -5, 10)), a));
			CAGE_TEST(intersects(makeSegment(vec3(-10, -12, -6), vec3(-5, -6, -2)), a));
			CAGE_TEST(!intersects(makeSegment(vec3(-4, -4, -10), vec3(-5, -5, -5)), a));
			CAGE_TEST(!intersects(makeSegment(vec3(-5, -5, -5), vec3(-4, -4, -5)), a));
		}

		{
			CAGE_TESTCASE("distance to point");
			aabb a(vec3(1, 3, 4), vec3(4, 7, 8));
			test(distance(a, vec3(0, 0, 0)), length(vec3(1, 3, 4)));
			test(distance(a, vec3(2, 7, 6)), 0);
			test(distance(a, vec3(3, 3, 10)), 2);
		}

		{
			CAGE_TESTCASE("transformation");
			aabb a(vec3(-5, -6, -3), vec3(-4, -4, -1));
			aabb b(vec3(1, 3, 4), vec3(4, 7, 8));
			aabb c(vec3(-10, -10, -10), vec3());
			aabb d(vec3(), vec3(10, 10, 10));
			aabb e(vec3(-5, -5, -5), vec3(5, 5, 5));
			mat4 rot1(quat(degs(30), degs(), degs()));
			mat4 rot2(quat(degs(), degs(315), degs()));
			mat4 tran(vec3(0, 10, 0));
			mat4 scl = mat4::scale(3);
			CAGE_TEST((a * rot1).volume() > a.volume());
			CAGE_TEST((a * rot1 * rot2).volume() > a.volume());
			CAGE_TEST((a * scl).volume() > a.volume());
			test((a * tran).volume(), a.volume());
		}

		{
			CAGE_TESTCASE("frustum culling");
			aabb a(vec3(1, 1, -7), vec3(3, 5, -1));
			mat4 proj = perspectiveProjection(degs(90), 1, 2, 10);
			// varying distance along z-axis
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(2, 3, -10)))) == false);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(2, 3, -5)))) == true);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(2, 3, 0)))) == true);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(2, 3, 3)))) == true);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(2, 3, 10)))) == false);
			// box moved left and right
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(-10, 3, 0)))) == false);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(0, 3, 0)))) == true);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(5, 3, 0)))) == true);
			CAGE_TEST(frustumCulling(a, proj * inverse(mat4(vec3(15, 3, 0)))) == false);
		}

		{
			CAGE_TESTCASE("triangle-aabb intersects");
			{
				aabb box(vec3(5, 3, 8), vec3(12, 9, 10));
				CAGE_TEST(intersects(triangle(vec3(6, 7, 8), vec3(11, 3, 8), vec3(11, 9, 10)), box)); // triangle fully inside
				CAGE_TEST(!intersects(triangle(vec3(-6, 7, 8), vec3(-11, 3, 8), vec3(-11, 9, 10)), box)); // triangle fully outside
			}
			{ // triangles with all vertices outside
				aabb box(vec3(0, 0, 0), vec3(2, 2, 2));
				CAGE_TEST(intersects(triangle(vec3(-1, -1, 1), vec3(3, -1, 1), vec3(1, 3, 1)), box)); // plane cut
				CAGE_TEST(intersects(triangle(vec3(0, -1, 1), vec3(3, -1, 1), vec3(3, 2, 1)), box)); // one edge
				CAGE_TEST(intersects(triangle(vec3(0, 0, 5), vec3(0, 5, 0), vec3(5, 0, 0)), box)); // cut a corner
				CAGE_TEST(intersects(triangle(vec3(-5, 1, 1), vec3(5, 1, 1), vec3(5, 1, 3)), box)); // needle
			}
		}

		{
			CAGE_TESTCASE("stringize");
			string s = detail::StringizerBase<5432>() + makeSegment(vec3(1, 2, 3), vec3(4, 5, 6)) + ", " + triangle() + ", " + plane() + ", " + sphere() + ", " + aabb();
		}
	}
}
