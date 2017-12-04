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

		vec3 closestPointOnTriangle(const triangle &trig, const vec3 &sourcePosition)
		{
			vec3 edge0 = trig[1] - trig[0];
			vec3 edge1 = trig[2] - trig[0];
			vec3 v0 = trig[0] - sourcePosition;
			real a = edge0.dot( edge0 );
			real b = edge0.dot( edge1 );
			real c = edge1.dot( edge1 );
			real d = edge0.dot( v0 );
			real e = edge1.dot( v0 );
			real det = a*c - b*b;
			real s = b*e - c*d;
			real t = b*d - a*e;
			if ( s + t < det )
			{
				if ( s < 0.f )
				{
					if ( t < 0.f )
					{
						if ( d < 0.f )
						{
							s = clamp( -d/a, 0.f, 1.f );
							t = 0.f;
						}
						else
						{
							s = 0.f;
							t = clamp( -e/c, 0.f, 1.f );
						}
					}
					else
					{
						s = 0.f;
						t = clamp( -e/c, 0.f, 1.f );
					}
				}
				else if ( t < 0.f )
				{
					s = clamp( -d/a, 0.f, 1.f );
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
				if ( s < 0.f )
				{
					real tmp0 = b+d;
					real tmp1 = c+e;
					if ( tmp1 > tmp0 )
					{
						real numer = tmp1 - tmp0;
						real denom = a-2*b+c;
						s = clamp( numer/denom, 0.f, 1.f );
						t = 1-s;
					}
					else
					{
						t = clamp( -e/c, 0.f, 1.f );
						s = 0.f;
					}
				}
				else if ( t < 0.f )
				{
					if ( a+d > b+e )
					{
						real numer = c+e-b-d;
						real denom = a-2*b+c;
						s = clamp( numer/denom, 0.f, 1.f );
						t = 1-s;
					}
					else
					{
						s = clamp( -e/c, 0.f, 1.f );
						t = 0.f;
					}
				}
				else
				{
					real numer = c+e-b-d;
					real denom = a-2*b+c;
					s = clamp( numer/denom, 0.f, 1.f );
					t = 1.f - s;
				}
			}
			return trig[0] + s * edge0 + t * edge1;
		}

		vec3 closestPointOnPlane(const plane &pl, const vec3 &pt)
		{
			return dot(pt, pl.normal) / dot(pl.normal, pl.normal) * pl.normal;
		}

		bool rayTriangle(triangle tri, const line &ray, real &t)
		{
			vec3 v0 = tri[0];
			vec3 v1 = tri[1];
			vec3 v2 = tri[2];
			vec3 edge1 = v1 - v0;
			vec3 edge2 = v2 - v0;
			vec3 pvec = cross(ray.origin, edge2);
			real det = dot(edge1, pvec);
			real invDet = 1 / det;
			vec3 tvec = ray.origin - v0;
			real u = dot(tvec, pvec) * invDet;
			if (u < 0 || u > 1)
				return false;
			vec3 qvec = cross(tvec, edge1);
			real v = dot(ray.direction, qvec) * invDet;
			if (v < 0 || u + v > 1)
				return false;
			t = dot(edge2, qvec) * invDet;
			if (t < ray.minimum || t > ray.maximum)
				return false;
			return true;
		}
	}






	bool parallel(const vec3 &dir1, const vec3 &dir2)
	{
		return abs(dot(dir1, dir2)) >= 1 - real::epsilon;
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
		return abs(dot(dir1, dir2)) <= real::epsilon;
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






	real distance(const vec3 &a, const line &b)
	{
		return distance(makeSegment(a, a), b);
	}

	real distance(const vec3 &a, const triangle &b)
	{
		return distance(closestPointOnTriangle(b, a), a);
	}

	real distance(const vec3 &a, const plane &b)
	{
		return distance(closestPointOnPlane(b, a), a);
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
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const line &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
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
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const plane &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	real distance(const triangle &a, const sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const triangle &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
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
		return distance(point, other) <= real::epsilon;
	}

	bool intersects(const vec3 &point, const triangle &other)
	{
		return distance(point, other) <= real::epsilon;
	}

	bool intersects(const vec3 &point, const plane &other)
	{
		return distance(point, other) <= real::epsilon;
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
		return distance(a, b) <= real::epsilon;
	}

	bool intersects(const line &a, const triangle &b)
	{
		real t;
		return rayTriangle(b, a, t);
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

	bool intersects(const triangle &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const plane &a, const plane &b)
	{
		if (parallel(a, b))
			return abs(a.d - b.d) < real::epsilon;
		return true;
	}

	bool intersects(const plane &a, const sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(const plane &a, const aabb &b)
	{
		CAGE_THROW_CRITICAL(notImplementedException, "geometry");
	}

	bool intersects(const sphere &a, const sphere &b)
	{
		return distance(a.center, b.center) <= (a.radius + b.radius);
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
