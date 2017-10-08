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







	real distance(const vec3 &a, const line &b)
	{
		return distance(makeSegment(a, a), b);
	}

	real distance(const vec3 &a, const triangle &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const vec3 &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const vec3 &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
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
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const line &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const line &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const line &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const triangle &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const plane &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const plane &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const plane &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const sphere &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const sphere &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const aabb &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const vec3 &point, const line &other)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const vec3 &point, const triangle &other)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const vec3 &point, const plane &other)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const vec3 &point, const sphere &other)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const vec3 &point, const aabb &other)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const line &a, const line &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const line &a, const triangle &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const line &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const line &a, const sphere &b)
	{
		return intersection(a, b).valid();
	}

	bool intersects(const line &a, const aabb &b)
	{
		return intersection(a, b).valid();
	}

	bool intersects(const triangle &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const triangle &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const triangle &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const plane &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const plane &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const plane &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const sphere &a, const sphere &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
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
}
