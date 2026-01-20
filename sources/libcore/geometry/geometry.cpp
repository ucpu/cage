#include <cage-core/geometry.h>

#if defined(_MSC_VER) && _MSC_VER > 1930 && _MSC_VER < 1939 && !defined(__clang__)
	// workaround for bug in visual studio 2022 compiler
	// the compiler either crashes, or generates code that produces invalid outputs from some functions
	#pragma optimize("", off)
#endif

namespace cage
{
	namespace
	{
		CAGE_FORCE_INLINE bool parallel(Vec3 dir1, Vec3 dir2)
		{
			return abs(dot(dir1, dir2)) >= 1 - 1e-5;
		}

		CAGE_FORCE_INLINE bool perpendicular(Vec3 dir1, Vec3 dir2)
		{
			return abs(dot(dir1, dir2)) <= 1e-5;
		}

		/*
		CAGE_FORCE_INLINE Rads angle(Vec3 a, Vec3 b)
		{
			CAGE_ASSERT(abs(lengthSquared(a) - 1) < 1e-4);
			CAGE_ASSERT(abs(lengthSquared(b) - 1) < 1e-4);
			return acos(dot(a, b));
		}
		*/
	}

	Rads angle(Line a, Line b)
	{
		return angle(a.direction, b.direction);
	}

	Rads angle(Line a, Triangle b)
	{
		return Rads(Degs(90)) - angle(a.direction, b.normal());
	}

	Rads angle(Line a, Plane b)
	{
		return Rads(Degs(90)) - angle(a.direction, b.normal);
	}

	Rads angle(Triangle a, Triangle b)
	{
		return angle(a.normal(), b.normal());
	}

	Rads angle(Triangle a, Plane b)
	{
		return angle(a.normal(), b.normal);
	}

	Rads angle(Plane a, Plane b)
	{
		return angle(a.normal, b.normal);
	}

	Real distance(Vec3 a, Line b)
	{
		CAGE_ASSERT(b.normalized());
		return distance(closestPoint(b, a), a);
	}

	Real distance(Vec3 a, Triangle b)
	{
		return distance(closestPoint(b, a), a);
	}

	Real distance(Vec3 a, Plane b)
	{
		return distance(closestPoint(b, a), a);
	}

	Real distance(Vec3 a, Sphere b)
	{
		return max(distance(a, b.center) - b.radius, Real(0));
	}

	Real distance(Vec3 a, Aabb b)
	{
		Vec3 c = max(min(a, b.b), b.a);
		return distance(c, a);
	}

	Real distance(Vec3 a, Cone b)
	{
		return distance(a, closestPoint(a, b));
	}

	Real distance(Vec3 a, const Frustum &b)
	{
		return distance(a, closestPoint(a, b));
	}

	Real distance(Line a, Triangle b)
	{
		Real d = Real::Infinity();
		if (a.isSegment() || a.isRay() || a.isPoint())
			d = min(d, distance(a.a(), b));
		if (a.isSegment())
			d = min(d, distance(a.b(), b));

		Vec3 p = intersection(Plane(b), makeLine(a.origin, a.origin + a.direction));
		if (!p.valid())
		{
			CAGE_ASSERT(perpendicular(a.direction, b.normal()));
			for (uint32 i = 0; i < 3; i++)
				d = min(d, distance(a, makeSegment(b[i], b[(i + 1) % 3])));
			return d;
		}
		Vec3 q = closestPoint(b, p);
		return min(distance(q, a), d);
	}

	Real distance(Line a, Plane b)
	{
		if (intersects(a, b))
			return 0;
		return min(distance(a.a(), b), distance(a.b(), b));
	}

	Real distance(Line a, Sphere b)
	{
		return max(distance(a, b.center) - b.radius, Real(0));
	}

	Real distance(Line a, Aabb b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Line, Aabb)");
	}

	Real distance(Line a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Line, Cone)");
	}

	Real distance(Line a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Line, Frustum)");
	}

	Real distance(Triangle a, Triangle b)
	{
		if (intersects(a, b))
			return 0;
		Real d = Real::Infinity();
		for (uint32 i = 0; i < 3; i++)
		{
			d = min(d, distance(makeSegment(a[i], a[(i + 1) % 3]), b));
			d = min(d, distance(a, makeSegment(b[i], b[(i + 1) % 3])));
		}
		return d;
	}

	Real distance(Triangle a, Plane b)
	{
		if (intersects(a, b))
			return 0;
		return min(min(distance(a[0], b), distance(a[1], b)), distance(a[2], b));
	}

	Real distance(Triangle a, Sphere b)
	{
		return max(distance(a, b.center) - b.radius, Real(0));
	}

	Real distance(Triangle a, Aabb b)
	{
		if (intersects(a, b))
			return 0;
		const Vec3 v[8] = { Vec3(b.a[0], b.a[1], b.a[2]), Vec3(b.b[0], b.a[1], b.a[2]), Vec3(b.a[0], b.b[1], b.a[2]), Vec3(b.b[0], b.b[1], b.a[2]), Vec3(b.a[0], b.a[1], b.b[2]), Vec3(b.b[0], b.a[1], b.b[2]), Vec3(b.a[0], b.b[1], b.b[2]), Vec3(b.b[0], b.b[1], b.b[2]) };
		static constexpr const uint32 ids[12 * 3] = { 0, 1, 2, 1, 2, 3, 4, 5, 6, 5, 6, 7, 0, 1, 4, 1, 4, 5, 2, 3, 6, 3, 6, 7, 0, 2, 4, 2, 4, 6, 1, 3, 5, 3, 5, 7 };
		Real d = Real::Infinity();
		for (uint32 i = 0; i < 12; i++)
		{
			const Triangle t(v[ids[i * 3 + 0]], v[ids[i * 3 + 1]], v[ids[i * 3 + 2]]);
			d = min(d, distance(t, a));
		}
		return d;
	}

	Real distance(Triangle a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Triangle, Cone)");
	}

	Real distance(Triangle a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Triangle, Frustum)");
	}

	Real distance(Plane a, Plane b)
	{
		CAGE_ASSERT(a.normalized() && b.normalized());
		if (parallel(a.normal, b.normal))
			return abs(a.d - b.d);
		return 0;
	}

	Real distance(Plane a, Sphere b)
	{
		return max(distance(a, b.center) - b.radius, Real(0));
	}

	Real distance(Plane a, Aabb b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Plane, Aabb)");
	}

	Real distance(Plane a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Plane, Cone)");
	}

	Real distance(Plane a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Plane, Frustum)");
	}

	Real distance(Sphere a, Sphere b)
	{
		return max(distance(a.center, b.center) - a.radius - b.radius, Real(0));
	}

	Real distance(Sphere a, Aabb b)
	{
		return max(distance(a.center, b) - a.radius, Real(0));
	}

	Real distance(Sphere a, Cone b)
	{
		return max(distance(a.center, b) - a.radius, Real(0));
	}

	Real distance(Sphere a, const Frustum &b)
	{
		return max(distance(a.center, b) - a.radius, Real(0));
	}

	Real distance(Aabb a, Aabb b)
	{
		// https://groups.google.com/g/comp.graphics.algorithms/c/hbu10Xc7JcY modified
		Real res = 0;
		for (int i = 0; i < 3; ++i)
		{
			if (a.a[i] >= b.b[i])
				res += sqr(a.a[i] - b.a[i]);
			else if (b.a[i] >= a.b[i])
				res += sqr(b.a[i] - a.b[i]);
		}
		return sqrt(res);
	}

	Real distance(Aabb a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Aabb, Cone)");
	}

	Real distance(Aabb a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Aabb, Frustum)");
	}

	Real distance(Cone a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Cone, Cone)");
	}

	Real distance(Cone a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Cone, Frustum)");
	}

	Real distance(const Frustum &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Frustum, Frustum)");
	}

	bool intersects(Vec3 a, Vec3 b)
	{
		return distance(a, b) <= 1e-5;
	}

	bool intersects(Vec3 point, Line other)
	{
		return distance(point, other) <= 1e-5;
	}

	bool intersects(Vec3 point, Triangle other)
	{
		return distance(point, other) <= 1e-5;
	}

	bool intersects(Vec3 point, Plane other)
	{
		return distance(point, other) <= 1e-5;
	}

	bool intersects(Vec3 point, Sphere other)
	{
		return distanceSquared(point, other.center) <= sqr(other.radius);
	}

	bool intersects(Vec3 point, Aabb other)
	{
		for (uint32 i = 0; i < 3; i++)
		{
			if (point[i] < other.a[i] || point[i] > other.b[i])
				return false;
		}
		return true;
	}

	bool intersects(Vec3 a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Vec3, Cone)");
	}

	bool intersects(Vec3 point, const Frustum &frustum)
	{
		for (uint32 i = 0; i < 6; i++)
		{
			const Vec4 &p = frustum.planes[i];
			const Real d = dot(Vec3(p), point);
			if (d < -p[3])
				return false;
		}
		return true;
	}

	bool intersects(Line a, Line b)
	{
		return distance(a, b) <= 1e-5;
	}

	bool intersects(Line ray, Triangle tri)
	{
		Vec3 v0 = tri[0];
		Vec3 v1 = tri[1];
		Vec3 v2 = tri[2];
		Vec3 edge1 = v1 - v0;
		Vec3 edge2 = v2 - v0;
		Vec3 pvec = cross(ray.direction, edge2);
		Real det = dot(edge1, pvec);
		if (abs(det) < 1e-5)
			return false;
		Real invDet = 1 / det;
		Vec3 tvec = ray.origin - v0;
		Real u = dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1)
			return false;
		Vec3 qvec = cross(tvec, edge1);
		Real v = dot(ray.direction, qvec) * invDet;
		if (v < 0 || u + v > 1)
			return false;
		Real t = dot(edge2, qvec) * invDet;
		if (t < ray.minimum || t > ray.maximum)
			return false;
		return true;
	}

	bool intersects(Line a, Plane b)
	{
		if (a.isLine())
		{
			if (perpendicular(a.direction, b.normal))
				return distance(a.origin, b) < 1e-5;
			return true;
		}
		else if (a.isRay())
		{
			if (perpendicular(a.direction, b.normal))
				return distance(a.origin, b) < 1e-5;
			const Real x = dot(a.origin - b.origin(), b.normal);
			const Real y = dot(a.direction, b.normal);
			return x < 0 != y < 0;
		}
		else
		{
			const Real x = dot(a.a() - b.origin(), b.normal);
			const Real y = dot(a.b() - b.origin(), b.normal);
			return x == 0 || y == 0 || (x < 0 != y < 0);
		}
	}

	bool intersects(Line a, Sphere b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(Line a, Aabb b)
	{
		return intersection(a, b).valid();
	}

	bool intersects(Line a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Line, Cone)");
	}

	bool intersects(Line a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Line, Frustum)");
	}

	bool intersects(Triangle a, Plane b)
	{
		uint32 sigs[3] = { 0, 0, 0 };
		for (uint32 i = 0; i < 3; i++)
			sigs[sign(dot(a[i] - b.origin(), b.normal)) + 1]++;
		sigs[0] += sigs[1];
		sigs[2] += sigs[1];
		return sigs[0] > 0 && sigs[2] > 0;
	}

	bool intersects(Triangle a, Sphere b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(Triangle a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Triangle, Cone)");
	}

	bool intersects(Triangle a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Triangle, Frustum)");
	}

	bool intersects(Plane a, Plane b)
	{
		CAGE_ASSERT(a.normalized() && b.normalized());
		if (parallel(a.normal, b.normal))
			return abs(a.d - b.d) < 1e-5;
		return true;
	}

	bool intersects(Plane a, Sphere b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(Plane a, Aabb b)
	{
		Vec3 c = b.center();
		Vec3 e = b.size() * 0.5;
		Real r = dot(e, abs(a.normal));
		Real s = dot(a.normal, c) - a.d;
		return abs(s) <= r;
	}

	bool intersects(Plane a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Plane, Cone)");
	}

	bool intersects(Plane a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Plane, Frustum)");
	}

	bool intersects(Sphere a, Sphere b)
	{
		return distanceSquared(a.center, b.center) <= sqr(a.radius + b.radius);
	}

	bool intersects(Sphere a, Aabb b)
	{
		return distance(a.center, b) <= a.radius;
	}

	bool intersects(Sphere sphere, Cone cone)
	{
		// https://bartwronski.com/2017/04/13/cull-that-cone/ modified
		const Vec3 V = sphere.center - cone.origin;
		const Real VlenSq = dot(V, V);
		const Real V1len = dot(V, cone.direction);
		const Real distanceClosestPoint = cos(cone.halfAngle) * sqrt(VlenSq - V1len * V1len) - V1len * sin(cone.halfAngle);
		const bool angleCull = distanceClosestPoint > sphere.radius;
		const bool frontCull = V1len > sphere.radius + cone.length;
		const bool backCull = V1len < -sphere.radius;
		return !(angleCull || frontCull || backCull);
	}

	bool intersects(Sphere sphere, const Frustum &frustum)
	{
		return intersects(Aabb(sphere), frustum);

		// the following seems broken
		// https://www.flipcode.com/archives/Frustum_Culling.shtml modified
		for (int i = 0; i < 6; i++)
		{
			const Real d = dot(frustum.planes[i], Vec4(sphere.center, 1));
			if (d < -sphere.radius)
				return false;
		}
		return true;
	}

	bool intersects(Aabb a, Aabb b)
	{
		if (a.empty() || b.empty())
			return false;
		for (uint32 i = 0; i < 3; i++)
		{
			if (a.b.data[i] < b.a.data[i])
				return false;
			if (a.a.data[i] > b.b.data[i])
				return false;
		}
		return true;
	}

	bool intersects(Aabb box, const Frustum &frustum)
	{
		const Vec3 b[] = { box.a, box.b };
		for (uint32 i = 0; i < 6; i++)
		{
			const Vec4 &p = frustum.planes[i]; // current plane
			const Vec3 pv = Vec3( // current p-vertex
				b[!!(p[0] > 0)][0], b[!!(p[1] > 0)][1], b[!!(p[2] > 0)][2]);
			const Real d = dot(Vec3(p), pv);
			if (d < -p[3])
				return false;
		}
		return true;
	}

	bool intersects(Cone a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Cone, Cone)");
	}

	bool intersects(Cone a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersects(Cone, Frustum)");
	}

	Vec3 intersection(Line ray, Triangle tri)
	{
		Vec3 v0 = tri[0];
		Vec3 v1 = tri[1];
		Vec3 v2 = tri[2];
		Vec3 edge1 = v1 - v0;
		Vec3 edge2 = v2 - v0;
		Vec3 pvec = cross(ray.direction, edge2);
		Real det = dot(edge1, pvec);
		if (abs(det) < 1e-5)
			return Vec3::Nan();
		Real invDet = 1 / det;
		Vec3 tvec = ray.origin - v0;
		Real u = dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1)
			return Vec3::Nan();
		Vec3 qvec = cross(tvec, edge1);
		Real v = dot(ray.direction, qvec) * invDet;
		if (v < 0 || u + v > 1)
			return Vec3::Nan();
		Real t = dot(edge2, qvec) * invDet;
		if (t < ray.minimum || t > ray.maximum)
			return Vec3::Nan();
		return ray.origin + t * ray.direction;
	}

	Vec3 intersection(Line a, Plane b)
	{
		CAGE_ASSERT(a.normalized());
		Real numer = dot(b.origin() - a.origin, b.normal);
		Real denom = dot(a.direction, b.normal);
		if (abs(denom) < 1e-5)
		{
			// parallel
			if (abs(numer) < 1e-5)
				return a.a(); // any point of the line
			return Vec3::Nan(); // no intersection
		}
		Real d = numer / denom;
		if (d < a.minimum || d > a.maximum)
			return Vec3::Nan(); // no intersection
		return a.origin + a.direction * d; // single intersection
	}

	Line intersection(Line a, Sphere b)
	{
		CAGE_ASSERT(a.normalized());
		Vec3 l = b.center - a.origin;
		Real tca = dot(l, a.direction);
		Real d2 = dot(l, l) - sqr(tca);
		Real r2 = sqr(b.radius);
		if (d2 > r2)
			return Line();
		Real thc = sqrt(r2 - d2);
		Real t0 = tca - thc;
		Real t1 = tca + thc;
		CAGE_ASSERT(t1 >= t0);
		if (t0 > a.maximum || t1 < a.minimum)
			return Line();
		return Line(a.origin, a.direction, max(a.minimum, t0), min(a.maximum, t1));
	}

	Line intersection(Line a, Aabb b)
	{
		CAGE_ASSERT(a.normalized());
		Real tmin = a.minimum;
		Real tmax = a.maximum;
		for (uint32 i = 0; i < 3; i++)
		{
			Real t1 = (b.a.data[i] - a.origin.data[i]) / a.direction.data[i];
			Real t2 = (b.b.data[i] - a.origin.data[i]) / a.direction.data[i];
			tmin = max(tmin, min(t1, t2));
			tmax = min(tmax, max(t1, t2));
		}
		if (tmin <= tmax)
			return Line(a.origin, a.direction, tmin, tmax);
		else
			return Line();
	}

	Line intersection(Line a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersection(Line, Cone)");
	}

	Line intersection(Line a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented intersection(Line, Frustum)");
	}

	Aabb intersection(Aabb a, Aabb b)
	{
		if (intersects(a, b))
			return Aabb(max(a.a, b.a), min(a.b, b.b));
		else
			return Aabb();
	}

	Vec3 closestPoint(Vec3 p, Line l)
	{
		Real d = dot(p - l.origin, l.direction);
		d = clamp(d, l.minimum, l.maximum);
		return l.origin + l.direction * d;
	}

	Vec3 closestPoint(Vec3 sourcePosition, Triangle trig)
	{
		const Vec3 p = closestPoint(Plane(trig), sourcePosition);
		{
			const Vec3 a = trig[0] - p;
			const Vec3 b = trig[1] - p;
			const Vec3 c = trig[2] - p;
			const Vec3 u = cross(a, b);
			const Vec3 v = cross(b, c);
			const Vec3 w = cross(c, a);
			if (dot(u, v) > 0 && dot(u, w) > 0)
				return p; // p is inside the triangle
		}
		const Line ab = makeSegment(trig[0], trig[1]);
		const Line bc = makeSegment(trig[1], trig[2]);
		const Line ca = makeSegment(trig[2], trig[0]);
		const Vec3 pab = closestPoint(ab, p);
		const Vec3 pbc = closestPoint(bc, p);
		const Vec3 pca = closestPoint(ca, p);
		const Real dab = distanceSquared(p, pab);
		const Real dbc = distanceSquared(p, pbc);
		const Real dca = distanceSquared(p, pca);
		const Real dm = min(min(dab, dbc), dca);
		if (dm == dab)
			return pab;
		if (dm == dbc)
			return pbc;
		return pca;
	}

	Vec3 closestPoint(Vec3 pt, Plane pl)
	{
		CAGE_ASSERT(pl.normalized());
		return pt - dot(pl.normal, pt - pl.origin()) * pl.normal;
	}

	Vec3 closestPoint(Vec3 a, Sphere b)
	{
		if (distanceSquared(a, b.center) > sqr(b.radius))
			return normalize(a - b.center) * b.radius + b.center;
		return a;
	}

	Vec3 closestPoint(Vec3 a, Aabb b)
	{
		return clamp(a, b.a, b.b);
	}

	Vec3 closestPoint(Vec3 a, Cone b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented closestPoint(Vec3, Cone)");
	}

	Vec3 closestPoint(Vec3 a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(Exception, "not implemented closestPoint(Vec3, Frustum)");
	}
}
