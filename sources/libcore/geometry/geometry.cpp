#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>

namespace cage
{
	namespace
	{
		real distanceLines(const vec3 a1, const vec3 &a2, const vec3 &b1, const vec3 &b2)
		{
			// algorithm taken from http://geomalgorithms.com/a07-_distance.html and modified

			// Copyright 2001 softSurfer, 2012 Dan Sunday
			// This code may be freely used, distributed and modified for any purpose
			// providing that this copyright notice is included with it.
			// SoftSurfer makes no warranty for this code, and cannot be held
			// liable for any real or imagined damage resulting from its use.
			// Users of this code must verify correctness for their application.

			vec3   u = a2 - a1;
			vec3   v = b2 - b1;
			vec3   w = a1 - b1;
			real    a = dot(u, u);
			real    b = dot(u, v);
			real    c = dot(v, v);
			real    d = dot(u, w);
			real    e = dot(v, w);
			real    D = a*c - b*b;
			real    sc, tc;
			if (D < 1e-6)
			{
				sc = 0.0;
				tc = (b > c ? d / b : e / c);
			}
			else
			{
				sc = (b*e - c*d) / D;
				tc = (a*e - b*d) / D;
			}
			vec3 dP = w + (sc * u) - (tc * v);
			return length(dP);
		}

		real distanceSegments(const vec3 a1, const vec3 &a2, const vec3 &b1, const vec3 &b2)
		{
			// algorithm taken from http://geomalgorithms.com/a07-_distance.html and modified

			// Copyright 2001 softSurfer, 2012 Dan Sunday
			// This code may be freely used, distributed and modified for any purpose
			// providing that this copyright notice is included with it.
			// SoftSurfer makes no warranty for this code, and cannot be held
			// liable for any real or imagined damage resulting from its use.
			// Users of this code must verify correctness for their application.

			vec3 u = a2 - a1;
			vec3 v = b2 - b1;
			vec3 w = a1 - b1;
			real a = dot(u, u);
			real b = dot(u, v);
			real c = dot(v, v);
			real d = dot(u, w);
			real e = dot(v, w);
			real D = a*c - b*b;
			real sc, sN, sD = D;
			real tc, tN, tD = D;
			if (D < 1e-6)
			{
				sN = 0.0;
				sD = 1.0;
				tN = e;
				tD = c;
			}
			else
			{
				sN = (b*e - c*d);
				tN = (a*e - b*d);
				if (sN < 0.0)
				{
					sN = 0.0;
					tN = e;
					tD = c;
				}
				else if (sN > sD)
				{
					sN = sD;
					tN = e + b;
					tD = c;
				}
			}
			if (tN < 0.0)
			{
				tN = 0.0;
				if (-d < 0.0)
					sN = 0.0;
				else if (-d > a)
					sN = sD;
				else {
					sN = -d;
					sD = a;
				}
			}
			else if (tN > tD)
			{
				tN = tD;
				if ((-d + b) < 0.0)
					sN = 0;
				else if ((-d + b) > a)
					sN = sD;
				else {
					sN = (-d + b);
					sD = a;
				}
			}
			sc = (abs(sN) < 1e-6 ? 0.0 : sN / sD);
			tc = (abs(tN) < 1e-6 ? 0.0 : tN / tD);
			vec3 dP = w + (sc * u) - (tc * v);
			return length(dP);
		}
	}







	bool parallel(const vec3 &dir1, const vec3 &dir2)
	{
		return abs(dot(dir1, dir2)) >= 1 - 1e-7;
	}

	bool parallel(const line &a, const line &b)
	{
		return parallel(a.direction, b.direction);
	}

	bool parallel(const line &a, const triangle &b)
	{
		return perpendicular(b.normal(), a.direction);
	}

	bool parallel(const line &a, const plane &b)
	{
		return perpendicular(b.normal, a.direction);
	}

	bool parallel(const triangle &a, const triangle &b)
	{
		return parallel(a.normal(), b.normal());
	}

	bool parallel(const triangle &a, const plane &b)
	{
		return parallel(a.normal(), b.normal);
	}

	bool parallel(const plane &a, const plane &b)
	{
		return parallel(a.normal, b.normal);
	}

	bool perpendicular(const vec3 &dir1, const vec3 &dir2)
	{
		return abs(dot(dir1, dir2)) <= 1e-7;
	}

	bool perpendicular(const line &a, const line &b)
	{
		return perpendicular(a.direction, b.direction);
	}

	bool perpendicular(const line &a, const triangle &b)
	{
		return parallel(a.direction, b.normal());
	}

	bool perpendicular(const line &a, const plane &b)
	{
		return parallel(a.direction, b.normal);
	}

	bool perpendicular(const triangle &a, const triangle &b)
	{
		return perpendicular(a.normal(), b.normal());
	}

	bool perpendicular(const triangle &a, const plane &b)
	{
		return perpendicular(a.normal(), b.normal);
	}

	bool perpendicular(const plane &a, const plane &b)
	{
		return perpendicular(a.normal, b.normal);
	}






	namespace
	{
		rads angle(const vec3 &a, const vec3 &b)
		{
			CAGE_ASSERT_RUNTIME(abs(a.squaredLength() - 1) < 1e-4);
			CAGE_ASSERT_RUNTIME(abs(b.squaredLength() - 1) < 1e-4);
			return aCos(dot(a, b));
		}
	}

	rads angle(const line &a, const line &b)
	{
		return angle(a.direction, b.direction);
	}

	rads angle(const line &a, const triangle &b)
	{
		return rads(degs(90)) - angle(a.direction, b.normal());
	}

	rads angle(const line &a, const plane &b)
	{
		return rads(degs(90)) - angle(a.direction, b.normal);
	}

	rads angle(const triangle &a, const triangle &b)
	{
		return angle(a.normal(), b.normal());
	}

	rads angle(const triangle &a, const plane &b)
	{
		return angle(a.normal(), b.normal);
	}

	rads angle(const plane &a, const plane &b)
	{
		return angle(a.normal, b.normal);
	}







	real distance(const vec3 &a, const line &b)
	{
		return distance(makeSegment(a, a), b);
	}

	real distance(const vec3 &a, const triangle &b)
	{
		return distance(closestPoint(b, a), a);
	}

	real distance(const vec3 &a, const plane &b)
	{
		return distance(closestPoint(b, a), a);
	}

	real distance(const vec3 &a, const sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const vec3 &a, const aabb &b)
	{
		vec3 c = max(min(a, b.b), b.a);
		return c.distance(a);
	}

	real distance(const line &a, const line &b)
	{
		if (a.isLine() && b.isLine())
			return distanceLines(a.a(), a.b(), b.a(), b.b());
		if (a.isSegment() && b.isSegment())
			return distanceSegments(a.a(), a.b(), b.a(), b.b());
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const line &a, const triangle &b)
	{
		if (intersects(a, b))
			return 0;
		return min(min(distance(a, makeSegment(b[0], b[1])),
			distance(a, makeSegment(b[1], b[2]))),
			distance(a, makeSegment(b[2], b[0])));
	}

	real distance(const line &a, const plane &b)
	{
		if (parallel(a, b))
			return distance(a.a(), b);
		return min(distance(a.a(), b), distance(a.b(), b));
	}

	real distance(const line &a, const sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const line &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const triangle &b)
	{
		if (intersects(a, b))
			return 0;
		real d = real::Infinity();
		for (uint32 i = 0; i < 3; i++)
		{
			d = min(d, distance(a[i], b));
			d = min(d, distance(a, b[i]));
		}
		return d;
	}

	real distance(const triangle &a, const plane &b)
	{
		if (intersects(a, b))
			return 0;
		return min(min(
			distance(a[0], b),
			distance(a[1], b)),
			distance(a[2], b)
		);
	}

	real distance(const triangle &a, const sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const triangle &a, const aabb &b)
	{
		if (intersects(a, b))
			return 0;
		vec3 v[8] = {
			vec3(b.a[0], b.a[1], b.a[2]),
			vec3(b.b[0], b.a[1], b.a[2]),
			vec3(b.a[0], b.b[1], b.a[2]),
			vec3(b.b[0], b.b[1], b.a[2]),
			vec3(b.a[0], b.a[1], b.b[2]),
			vec3(b.b[0], b.a[1], b.b[2]),
			vec3(b.a[0], b.b[1], b.b[2]),
			vec3(b.b[0], b.b[1], b.b[2])
		};
		static const uint32 ids[12 * 3] =  {
			0, 1, 2,
			1, 2, 3,
			4, 5, 6,
			5, 6, 7,
			0, 1, 4,
			1, 4, 5,
			2, 3, 6,
			3, 6, 7,
			0, 2, 4,
			2, 4, 6,
			1, 3, 5,
			3, 5, 7
		};
		real d = real::Infinity();
		for (uint32 i = 0; i < 12; i++)
		{
			triangle t(
				v[ids[i * 3 + 0]],
				v[ids[i * 3 + 1]],
				v[ids[i * 3 + 2]]
			);
			d = min(d, distance(t, a));
		}
		return d;
	}

	real distance(const plane &a, const plane &b)
	{
		if (parallel(a, b))
			return a.d / a.normal.length() - b.d / b.normal.length();
		return 0;
	}

	real distance(const plane &a, const sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const plane &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const sphere &a, const sphere &b)
	{
		return max(distance(a.center, b.center) - a.radius - b.radius, real(0));
	}

	real distance(const sphere &a, const aabb &b)
	{
		return max(distance(a.center, b) - a.radius, real(0));
	}

	real distance(const aabb &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}







	bool intersects(const vec3 &point, const line &other)
	{
		return distance(point, other) <= 1e-7;
	}

	bool intersects(const vec3 &point, const triangle &other)
	{
		return distance(point, other) <= 1e-7;
	}

	bool intersects(const vec3 &point, const plane &other)
	{
		return distance(point, other) <= 1e-7;
	}

	bool intersects(const vec3 &point, const sphere &other)
	{
		return squaredDistance(point, other.center) <= sqr(other.radius);
	}

	bool intersects(const vec3 &point, const aabb &other)
	{
		for (uint32 i = 0; i < 3; i++)
		{
			if (point[i] < other.a[i] || point[i] > other.b[i])
				return false;
		}
		return true;
	}

	bool intersects(const line &a, const line &b)
	{
		return distance(a, b) <= 1e-7;
	}

	bool intersects(const line &ray, const triangle &tri)
	{
		vec3 v0 = tri[0];
		vec3 v1 = tri[1];
		vec3 v2 = tri[2];
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		vec3 pvec = cross(ray.direction, edge2);
		real det = dot(edge1, pvec);
		if (abs(det) < 1e-7)
			return false;
		real invDet = 1 / det;
		vec3 tvec = ray.origin - v0;
		real u = dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1)
			return false;
		vec3 qvec = cross(tvec, edge1);
		real v = dot(ray.direction, qvec) * invDet;
		if (v < 0 || u + v > 1)
			return false;
		real t = dot(edge2, qvec) * invDet;
		if (t < ray.minimum || t > ray.maximum)
			return false;
		return true;
	}

	bool intersects(const line &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const line &a, const sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(const line &a, const aabb &b)
	{
		return intersection(a, b).valid();
	}

	bool intersects(const triangle &a, const plane &b)
	{
		uint32 sigs[2] = { 0, 0 };
		for (uint32 i = 0; i < 3; i++)
			sigs[dot(a[i], b.normal) - b.d < 0]++;
		return sigs[0] > 0 && sigs[1] > 0;
	}

	bool intersects(const triangle &a, const sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	namespace
	{
		// https://gist.github.com/Philipp-M/e5747bd5a4e264ab143824059d21c120

		void findMinMax(real x0, real x1, real x2, real &min, real &max)
		{
			min = max = x0;
			if (x1 < min)
				min = x1;
			if (x1 > max)
				max = x1;
			if (x2 < min)
				min = x2;
			if (x2 > max)
				max = x2;
		}

		bool planeBoxOverlap(vec3 normal, vec3 vert, vec3 maxbox)
		{
			vec3 vmin, vmax;
			real v;
			for (uint32 q = 0; q < 3; q++)
			{
				v = vert[q];
				if (normal[q] > 0.0f) {
					vmin[q] = -maxbox[q] - v;
					vmax[q] = maxbox[q] - v;
				}
				else {
					vmin[q] = maxbox[q] - v;
					vmax[q] = -maxbox[q] - v;
				}
			}
			if (dot(normal, vmin) > 0.0f)
				return false;
			if (dot(normal, vmax) >= 0.0f)
				return true;
			return false;
		}

		bool axisTestX01(real a, real b, real fa, real fb, const vec3 &v0, const vec3 &v2, const vec3 &boxhalfsize, real &rad, real &min, real &max, real &p0, real &p2)
		{
			p0 = a * v0[1] - b * v0[2];
			p2 = a * v2[1] - b * v2[2];
			if (p0 < p2)
			{
				min = p0;
				max = p2;
			}
			else
			{
				min = p2;
				max = p0;
			}
			rad = fa * boxhalfsize[1] + fb * boxhalfsize[2];
			if (min > rad || max < -rad)
				return false;
			return true;
		}
		
		bool axisTestX2(real a, real b, real fa, real fb, const vec3 &v0, const vec3 &v1, const vec3 &boxhalfsize, real &rad, real &min, real &max, real &p0, real &p1)
		{
			p0 = a * v0[1] - b * v0[2];
			p1 = a * v1[1] - b * v1[2];
			if (p0 < p1)
			{
				min = p0;
				max = p1;
			}
			else
			{
				min = p1;
				max = p0;
			}
			rad = fa * boxhalfsize[1] + fb * boxhalfsize[2];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool axisTestY02(real a, real b, real fa, real fb, const vec3 &v0, const vec3 &v2, const vec3 &boxhalfsize, real &rad, real &min, real &max, real &p0, real &p2)
		{
			p0 = -a * v0[0] + b * v0[2];
			p2 = -a * v2[0] + b * v2[2];
			if (p0 < p2)
			{
				min = p0;
				max = p2;
			}
			else
			{
				min = p2;
				max = p0;
			}
			rad = fa * boxhalfsize[0] + fb * boxhalfsize[2];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool axisTestY1(real a, real b, real fa, real fb, const vec3 &v0, const vec3 &v1, const vec3 &boxhalfsize, real &rad, real &min, real &max, real &p0, real &p1)
		{
			p0 = -a * v0[0] + b * v0[2];
			p1 = -a * v1[0] + b * v1[2];
			if (p0 < p1)
			{
				min = p0;
				max = p1;
			}
			else
			{
				min = p1;
				max = p0;
			}
			rad = fa * boxhalfsize[0] + fb * boxhalfsize[2];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool axisTestZ12(real a, real b, real fa, real fb, const vec3 &v2, const vec3 &boxhalfsize, real &rad, real min, real max, real &p2)
		{
			p2 = a * v2[0] - b * v2[1];
			rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool axisTestZ0(real a, real b, real fa, real fb, const vec3 &v0, const vec3 &v1, const vec3 &boxhalfsize, real &rad, real &min, real &max, real &p0, real &p1)
		{
			p0 = a * v0[0] - b * v0[1];
			p1 = a * v1[0] - b * v1[1];
			if (p0 < p1) {
				min = p0;
				max = p1;
			}
			else {
				min = p1;
				max = p0;
			}
			rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool triBoxOverlap(vec3 boxcenter, vec3 boxhalfsize, vec3 tv0, vec3 tv1, vec3 tv2)
		{
			/*    use separating axis theorem to test overlap between triangle and box */
			/*    need to test for overlap in these directions: */
			/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
			/*       we do not even need to test these) */
			/*    2) normal of the triangle */
			/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
			/*       this gives 3x3=9 more tests */
			vec3 v0, v1, v2;
			real min, max, p0, p1, p2, rad, fex, fey, fez;
			vec3 normal, e0, e1, e2;

			/* This is the fastest branch on Sun */
			/* move everything so that the boxcenter is in (0,0,0) */
			v0 = tv0 - boxcenter;
			v1 = tv1 - boxcenter;
			v2 = tv2 - boxcenter;

			/* compute triangle edges */
			e0 = v1 - v0;
			e1 = v2 - v1;
			e2 = v0 - v2;

			/* Bullet 3:  */
			/*  test the 9 tests first (this was faster) */
			fex = abs(e0[0]);
			fey = abs(e0[1]);
			fez = abs(e0[2]);

			if (!axisTestX01(e0[2], e0[1], fez, fey, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestY02(e0[2], e0[0], fez, fex, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestZ12(e0[1], e0[0], fey, fex, v2, boxhalfsize, rad, min, max, p2))
				return false;

			fex = abs(e1[0]);
			fey = abs(e1[1]);
			fez = abs(e1[2]);

			if (!axisTestX01(e1[2], e1[1], fez, fey, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestY02(e1[2], e1[0], fez, fex, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestZ0(e1[1], e1[0], fez, fex, v0, v1, boxhalfsize, rad, min, max, p0, p1))
				return false;

			fex = abs(e2[0]);
			fey = abs(e2[1]);
			fez = abs(e2[2]);
			if (!axisTestX2(e2[2], e2[1], fez, fey, v0, v1, boxhalfsize, rad, min, max, p0, p1))
				return false;
			if (!axisTestY02(e2[2], e2[0], fez, fex, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestZ12(e2[1], e2[0], fey, fex, v2, boxhalfsize, rad, min, max, p2))
				return false;

			/* Bullet 1: */
			/*  first test overlap in the {x,y,z}-directions */
			/*  find min, max of the triangle each direction, and test for overlap in */
			/*  that direction -- this is equivalent to testing a minimal AABB around */
			/*  the triangle against the AABB */

			/* test in X-direction */
			findMinMax(v0[0], v1[0], v2[0], min, max);
			if (min > boxhalfsize[0] || max < -boxhalfsize[0])
				return false;

			/* test in Y-direction */
			findMinMax(v0[1], v1[1], v2[1], min, max);
			if (min > boxhalfsize[1] || max < -boxhalfsize[1])
				return false;

			/* test in Z-direction */
			findMinMax(v0[2], v1[2], v2[2], min, max);
			if (min > boxhalfsize[2] || max < -boxhalfsize[2])
				return false;

			/* Bullet 2: */
			/*  test if the box intersects the plane of the triangle */
			/*  compute plane equation of triangle: normal*x+d=0 */
			normal = cross(e0, e1);
			if (!planeBoxOverlap(normal, v0, boxhalfsize))
				return false;

			return true; /* box and triangle overlaps */
		}
	}

	bool intersects(const triangle &a, const aabb &b)
	{
		return triBoxOverlap(b.center(), b.size() * 0.5, a[0], a[1], a[2]);
	}

	bool intersects(const plane &a, const plane &b)
	{
		if (parallel(a, b))
			return abs(a.d - b.d) < 1e-7;
		return true;
	}

	bool intersects(const plane &a, const sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(const plane &a, const aabb &b)
	{
		vec3 c = b.center();
		vec3 e = b.size() * 0.5;
		real r = e[0] * abs(a.normal[0]) + e[1] * abs(a.normal[1]) + e[2] * abs(a.normal[2]);
		real s = dot(a.normal, c) - a.d;
		return abs(s) <= r;
	}

	bool intersects(const sphere &a, const sphere &b)
	{
		return squaredDistance(a.center, b.center) <= sqr(a.radius + b.radius);
	}

	bool intersects(const sphere &a, const aabb &b)
	{
		return distance(a.center, b) <= a.radius;
	}

	bool intersects(const aabb &a, const aabb &b)
	{
		if (a.empty() || b.empty())
			return false;
		for (uint32 i = 0; i < 3; i++)
		{
			if (a.b.data[i] < b.a.data[i]) return false;
			if (a.a.data[i] > b.b.data[i]) return false;
		}
		return true;
	}







	vec3 intersection(const line &ray, const triangle &tri)
	{
		vec3 v0 = tri[0];
		vec3 v1 = tri[1];
		vec3 v2 = tri[2];
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		vec3 pvec = cross(ray.direction, edge2);
		real det = dot(edge1, pvec);
		if (abs(det) < 1e-7)
			return vec3::Nan();
		real invDet = 1 / det;
		vec3 tvec = ray.origin - v0;
		real u = dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1)
			return vec3::Nan();
		vec3 qvec = cross(tvec, edge1);
		real v = dot(ray.direction, qvec) * invDet;
		if (v < 0 || u + v > 1)
			return vec3::Nan();
		real t = dot(edge2, qvec) * invDet;
		if (t < ray.minimum || t > ray.maximum)
			return vec3::Nan();
		return ray.origin + t * ray.direction;
	}

	vec3 intersection(const line &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	line intersection(const line &a, const sphere &b)
	{
		if (!a.valid() || !b.valid())
			return line();
		CAGE_ASSERT_RUNTIME(a.normalized());
		vec3 l = b.center - a.origin;
		real tca = l.dot(a.direction);
		real d2 = l.dot(l) - sqr(tca);
		real r2 = sqr(b.radius);
		if (d2 > r2)
			return line();
		real thc = sqrt(r2 - d2);
		real t0 = tca - thc;
		real t1 = tca + thc;
		CAGE_ASSERT_RUNTIME(t1 >= t0);
		if (t0 > a.maximum || t1 < a.minimum)
			return line();
		return line(a.origin, a.direction, max(a.minimum, t0), min(a.maximum, t1));
	}

	line intersection(const line &a, const aabb &b)
	{
		CAGE_ASSERT_RUNTIME(a.normalized());
		real tmin = a.minimum;
		real tmax = a.maximum;
		for (uint32 i = 0; i < 3; i++)
		{
			real t1 = (b.a.data[i] - a.origin.data[i]) / a.direction.data[i];
			real t2 = (b.b.data[i] - a.origin.data[i]) / a.direction.data[i];
			tmin = max(tmin, min(t1, t2));
			tmax = min(tmax, max(t1, t2));
		}
		if (tmin <= tmax)
			return line(a.origin, a.direction, tmin, tmax);
		else
			return line();
	}

	aabb intersection(const aabb &a, const aabb &b)
	{
		if (intersects(a, b))
			return aabb(max(a.a, b.a), min(a.b, b.b));
		else
			return aabb();
	}







	bool frustumCulling(const vec3 &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool frustumCulling(const line &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool frustumCulling(const triangle &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool frustumCulling(const plane &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool frustumCulling(const sphere &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	namespace
	{
		vec4 column(const mat4 &m, uint32 index)
		{
			return vec4(m[index], m[index + 4], m[index + 8], m[index + 12]);
		}
	}

	bool frustumCulling(const aabb &box, const mat4 &mvp)
	{
		vec4 planes[6] = {
			column(mvp, 3) + column(mvp, 0),
			column(mvp, 3) - column(mvp, 0),
			column(mvp, 3) + column(mvp, 1),
			column(mvp, 3) - column(mvp, 1),
			column(mvp, 3) + column(mvp, 2),
			column(mvp, 3) - column(mvp, 2),
		};
		const vec3 b[] = { box.a, box.b };
		for (uint32 i = 0; i < 6; i++)
		{
			const vec4 &p = planes[i]; // current plane
			const vec3 pv = vec3( // current p-vertex
				b[!!(p[0] > 0)][0],
				b[!!(p[1] > 0)][1],
				b[!!(p[2] > 0)][2]
			);
			const real d = dot(vec3(p), pv);
			if (d < -p[3])
				return false;
		}
		return true;
	}







	vec3 closestPoint(const triangle &trig, const vec3 &sourcePosition)
	{
		vec3 edge0 = trig[1] - trig[0];
		vec3 edge1 = trig[2] - trig[0];
		vec3 v0 = trig[0] - sourcePosition;
		real a = edge0.dot(edge0);
		real b = edge0.dot(edge1);
		real c = edge1.dot(edge1);
		real d = edge0.dot(v0);
		real e = edge1.dot(v0);
		real det = a * c - b * b;
		real s = b * e - c * d;
		real t = b * d - a * e;
		if (s + t < det)
		{
			if (s < 0.f)
			{
				if (t < 0.f)
				{
					if (d < 0.f)
					{
						s = clamp(-d / a, 0.f, 1.f);
						t = 0.f;
					}
					else
					{
						s = 0.f;
						t = clamp(-e / c, 0.f, 1.f);
					}
				}
				else
				{
					s = 0.f;
					t = clamp(-e / c, 0.f, 1.f);
				}
			}
			else if (t < 0.f)
			{
				s = clamp(-d / a, 0.f, 1.f);
				t = 0.f;
			}
			else
			{
				real invDet = 1.f / det;
				s *= invDet;
				t *= invDet;
			}
		}
		else
		{
			if (s < 0.f)
			{
				real tmp0 = b + d;
				real tmp1 = c + e;
				if (tmp1 > tmp0)
				{
					real numer = tmp1 - tmp0;
					real denom = a - 2 * b + c;
					s = clamp(numer / denom, 0.f, 1.f);
					t = 1 - s;
				}
				else
				{
					t = clamp(-e / c, 0.f, 1.f);
					s = 0.f;
				}
			}
			else if (t < 0.f)
			{
				if (a + d > b + e)
				{
					real numer = c + e - b - d;
					real denom = a - 2 * b + c;
					s = clamp(numer / denom, 0.f, 1.f);
					t = 1 - s;
				}
				else
				{
					s = clamp(-e / c, 0.f, 1.f);
					t = 0.f;
				}
			}
			else
			{
				real numer = c + e - b - d;
				real denom = a - 2 * b + c;
				s = clamp(numer / denom, 0.f, 1.f);
				t = 1.f - s;
			}
		}
		return trig[0] + s * edge0 + t * edge1;
	}

	vec3 closestPoint(const plane &pl, const vec3 &pt)
	{
		return dot(pt, pl.normal) / dot(pl.normal, pl.normal) * pl.normal;
	}
}
