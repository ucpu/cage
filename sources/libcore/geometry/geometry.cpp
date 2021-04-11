#include <cage-core/geometry.h>

namespace cage
{
	namespace
	{
		bool parallel(const vec3 &dir1, const vec3 &dir2)
		{
			return abs(dot(dir1, dir2)) >= 1 - 1e-5;
		}

		bool perpendicular(const vec3 &dir1, const vec3 &dir2)
		{
			return abs(dot(dir1, dir2)) <= 1e-5;
		}

		rads angle(const vec3 &a, const vec3 &b)
		{
			CAGE_ASSERT(abs(lengthSquared(a) - 1) < 1e-4);
			CAGE_ASSERT(abs(lengthSquared(b) - 1) < 1e-4);
			return acos(dot(a, b));
		}
	}



	rads angle(const Line &a, const Line &b)
	{
		return angle(a.direction, b.direction);
	}

	rads angle(const Line &a, const Triangle &b)
	{
		return rads(degs(90)) - angle(a.direction, b.normal());
	}

	rads angle(const Line &a, const Plane &b)
	{
		return rads(degs(90)) - angle(a.direction, b.normal);
	}

	rads angle(const Triangle &a, const Triangle &b)
	{
		return angle(a.normal(), b.normal());
	}

	rads angle(const Triangle &a, const Plane &b)
	{
		return angle(a.normal(), b.normal);
	}

	rads angle(const Plane &a, const Plane &b)
	{
		return angle(a.normal, b.normal);
	}



	real distance(const vec3 &a, const Line &b)
	{
		CAGE_ASSERT(b.normalized());
		return distance(closestPoint(b, a), a);
	}

	real distance(const vec3 &a, const Triangle &b)
	{
		return distance(closestPoint(b, a), a);
	}

	real distance(const vec3 &a, const Plane &b)
	{
		return distance(closestPoint(b, a), a);
	}

	real distance(const vec3 &a, const Sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const vec3 &a, const Aabb &b)
	{
		vec3 c = max(min(a, b.b), b.a);
		return distance(c, a);
	}

	real distance(const vec3 &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const vec3 &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Line &a, const Triangle &b)
	{
		real d = real::Infinity();
		if (a.isSegment() || a.isRay() || a.isPoint())
			d = min(d, distance(a.a(), b));
		if (a.isSegment())
			d = min(d, distance(a.b(), b));

		vec3 p = intersection(Plane(b), makeLine(a.origin, a.origin + a.direction));
		if (!p.valid())
		{
			CAGE_ASSERT(perpendicular(a.direction, b.normal()));
			for (uint32 i = 0; i < 3; i++)
				d = min(d, distance(a, makeSegment(b[i], b[(i + 1) % 3])));
			return d;
		}
		vec3 q = closestPoint(b, p);
		return min(distance(q, a), d);
	}

	real distance(const Line &a, const Plane &b)
	{
		if (intersects(a, b))
			return 0;
		return min(distance(a.a(), b), distance(a.b(), b));
	}

	real distance(const Line &a, const Sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const Line &a, const Aabb &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Line &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Line &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Triangle &a, const Triangle &b)
	{
		if (intersects(a, b))
			return 0;
		real d = real::Infinity();
		for (uint32 i = 0; i < 3; i++)
		{
			d = min(d, distance(makeSegment(a[i], a[(i + 1) % 3]), b));
			d = min(d, distance(a, makeSegment(b[i], b[(i + 1) % 3])));
		}
		return d;
	}

	real distance(const Triangle &a, const Plane &b)
	{
		if (intersects(a, b))
			return 0;
		return min(min(
			distance(a[0], b),
			distance(a[1], b)),
			distance(a[2], b)
		);
	}

	real distance(const Triangle &a, const Sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const Triangle &a, const Aabb &b)
	{
		if (intersects(a, b))
			return 0;
		const vec3 v[8] = {
			vec3(b.a[0], b.a[1], b.a[2]),
			vec3(b.b[0], b.a[1], b.a[2]),
			vec3(b.a[0], b.b[1], b.a[2]),
			vec3(b.b[0], b.b[1], b.a[2]),
			vec3(b.a[0], b.a[1], b.b[2]),
			vec3(b.b[0], b.a[1], b.b[2]),
			vec3(b.a[0], b.b[1], b.b[2]),
			vec3(b.b[0], b.b[1], b.b[2])
		};
		static constexpr const uint32 ids[12 * 3] =  {
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
			const Triangle t(
				v[ids[i * 3 + 0]],
				v[ids[i * 3 + 1]],
				v[ids[i * 3 + 2]]
			);
			d = min(d, distance(t, a));
		}
		return d;
	}

	real distance(const Triangle &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Triangle &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Plane &a, const Plane &b)
	{
		CAGE_ASSERT(a.normalized() && b.normalized());
		if (parallel(a.normal, b.normal))
			return abs(a.d - b.d);
		return 0;
	}

	real distance(const Plane &a, const Sphere &b)
	{
		return max(distance(a, b.center) - b.radius, real(0));
	}

	real distance(const Plane &a, const Aabb &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Plane &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Plane &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Sphere &a, const Sphere &b)
	{
		return max(distance(a.center, b.center) - a.radius - b.radius, real(0));
	}

	real distance(const Sphere &a, const Aabb &b)
	{
		return max(distance(a.center, b) - a.radius, real(0));
	}

	real distance(const Sphere &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Sphere &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Aabb &a, const Aabb &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Aabb &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Aabb &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Cone &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Cone &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	real distance(const Frustum &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}



	bool intersects(const vec3 &a, const vec3 &b)
	{
		return distance(a, b) <= 1e-5;
	}

	bool intersects(const vec3 &point, const Line &other)
	{
		return distance(point, other) <= 1e-5;
	}

	bool intersects(const vec3 &point, const Triangle &other)
	{
		return distance(point, other) <= 1e-5;
	}

	bool intersects(const vec3 &point, const Plane &other)
	{
		return distance(point, other) <= 1e-5;
	}

	bool intersects(const vec3 &point, const Sphere &other)
	{
		return distanceSquared(point, other.center) <= sqr(other.radius);
	}

	bool intersects(const vec3 &point, const Aabb &other)
	{
		for (uint32 i = 0; i < 3; i++)
		{
			if (point[i] < other.a[i] || point[i] > other.b[i])
				return false;
		}
		return true;
	}

	bool intersects(const vec3 &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const vec3 &point, const Frustum &frustum)
	{
		for (uint32 i = 0; i < 6; i++)
		{
			const vec4 &p = frustum.planes[i];
			const real d = dot(vec3(p), point);
			if (d < -p[3])
				return false;
		}
		return true;
	}

	bool intersects(const Line &a, const Line &b)
	{
		return distance(a, b) <= 1e-5;
	}

	bool intersects(const Line &ray, const Triangle &tri)
	{
		vec3 v0 = tri[0];
		vec3 v1 = tri[1];
		vec3 v2 = tri[2];
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		vec3 pvec = cross(ray.direction, edge2);
		real det = dot(edge1, pvec);
		if (abs(det) < 1e-5)
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

	bool intersects(const Line &a, const Plane &b)
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
			real x = dot(a.origin - b.origin(), b.normal);
			real y = dot(a.direction, b.normal);
			return x < 0 != y < 0;
		}
		else
		{
			real x = dot(a.a() - b.origin(), b.normal);
			real y = dot(a.b() - b.origin(), b.normal);
			return x == 0 || y == 0 || (x < 0 != y < 0);
		}
	}

	bool intersects(const Line &a, const Sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(const Line &a, const Aabb &b)
	{
		return intersection(a, b).valid();
	}

	bool intersects(const Line &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Line &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Triangle &a, const Plane &b)
	{
		uint32 sigs[2] = { 0, 0 };
		for (uint32 i = 0; i < 3; i++)
			sigs[dot(a[i], b.normal) - b.d < 0]++;
		return sigs[0] > 0 && sigs[1] > 0;
	}

	bool intersects(const Triangle &a, const Sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(const Triangle &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Triangle &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Plane &a, const Plane &b)
	{
		CAGE_ASSERT(a.normalized() && b.normalized());
		if (parallel(a.normal, b.normal))
			return abs(a.d - b.d) < 1e-5;
		return true;
	}

	bool intersects(const Plane &a, const Sphere &b)
	{
		return distance(a, b.center) <= b.radius;
	}

	bool intersects(const Plane &a, const Aabb &b)
	{
		vec3 c = b.center();
		vec3 e = b.size() * 0.5;
		real r = e[0] * abs(a.normal[0]) + e[1] * abs(a.normal[1]) + e[2] * abs(a.normal[2]);
		real s = dot(a.normal, c) - a.d;
		return abs(s) <= r;
	}

	bool intersects(const Plane &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Plane &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Sphere &a, const Sphere &b)
	{
		return distanceSquared(a.center, b.center) <= sqr(a.radius + b.radius);
	}

	bool intersects(const Sphere &a, const Aabb &b)
	{
		return distance(a.center, b) <= a.radius;
	}

	bool intersects(const Sphere &sphere, const Cone &cone)
	{
		// https://bartwronski.com/2017/04/13/cull-that-cone/ modified
		const vec3 V = sphere.center - cone.origin;
		const real VlenSq = dot(V, V);
		const real V1len = dot(V, cone.direction);
		const real distanceClosestPoint = cos(cone.halfAngle) * sqrt(VlenSq - V1len * V1len) - V1len * sin(cone.halfAngle);
		const bool angleCull = distanceClosestPoint > sphere.radius;
		const bool frontCull = V1len > sphere.radius + cone.length;
		const bool backCull = V1len < -sphere.radius;
		return !(angleCull || frontCull || backCull);
	}

	bool intersects(const Sphere &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Aabb &a, const Aabb &b)
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

	bool intersects(const Aabb &a, const ExactFrustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Aabb &box, const ConservativeFrustum &frustum)
	{
		const vec3 b[] = { box.a, box.b };
		for (uint32 i = 0; i < 6; i++)
		{
			const vec4 &p = frustum.planes[i]; // current plane
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

	bool intersects(const Cone &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Cone &a, const ExactFrustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool intersects(const Cone &a, const ConservativeFrustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}



	vec3 intersection(const Line &ray, const Triangle &tri)
	{
		vec3 v0 = tri[0];
		vec3 v1 = tri[1];
		vec3 v2 = tri[2];
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		vec3 pvec = cross(ray.direction, edge2);
		real det = dot(edge1, pvec);
		if (abs(det) < 1e-5)
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

	vec3 intersection(const Line &a, const Plane &b)
	{
		CAGE_ASSERT(a.normalized());
		real numer = dot(b.origin() - a.origin, b.normal);
		real denom = dot(a.direction, b.normal);
		if (abs(denom) < 1e-5)
		{
			// parallel
			if (abs(numer) < 1e-5)
				return a.a(); // any point of the Line
			return vec3::Nan(); // no intersection
		}
		real d = numer / denom;
		if (d < a.minimum || d > a.maximum)
			return vec3::Nan(); // no intersection
		return a.origin + a.direction * d; // single intersection
	}

	Line intersection(const Line &a, const Sphere &b)
	{
		CAGE_ASSERT(a.normalized());
		vec3 l = b.center - a.origin;
		real tca = dot(l, a.direction);
		real d2 = dot(l, l) - sqr(tca);
		real r2 = sqr(b.radius);
		if (d2 > r2)
			return Line();
		real thc = sqrt(r2 - d2);
		real t0 = tca - thc;
		real t1 = tca + thc;
		CAGE_ASSERT(t1 >= t0);
		if (t0 > a.maximum || t1 < a.minimum)
			return Line();
		return Line(a.origin, a.direction, max(a.minimum, t0), min(a.maximum, t1));
	}

	Line intersection(const Line &a, const Aabb &b)
	{
		CAGE_ASSERT(a.normalized());
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
			return Line(a.origin, a.direction, tmin, tmax);
		else
			return Line();
	}

	Aabb intersection(const Aabb &a, const Aabb &b)
	{
		if (intersects(a, b))
			return Aabb(max(a.a, b.a), min(a.b, b.b));
		else
			return Aabb();
	}



	vec3 closestPoint(const vec3 &p, const Line &l)
	{
		real d = dot(p - l.origin, l.direction);
		d = clamp(d, l.minimum, l.maximum);
		return l.origin + l.direction * d;
	}

	vec3 closestPoint(const vec3 &sourcePosition, const Triangle &trig)
	{
		const vec3 p = closestPoint(Plane(trig), sourcePosition);
		{
			const vec3 a = trig[0] - p;
			const vec3 b = trig[1] - p;
			const vec3 c = trig[2] - p;
			const vec3 u = cross(a, b);
			const vec3 v = cross(b, c);
			const vec3 w = cross(c, a);
			if (dot(u, v) > 0 && dot(u, w) > 0)
				return p; // p is inside the Triangle
		}
		const Line ab = makeSegment(trig[0], trig[1]);
		const Line bc = makeSegment(trig[1], trig[2]);
		const Line ca = makeSegment(trig[2], trig[0]);
		const vec3 pab = closestPoint(ab, p);
		const vec3 pbc = closestPoint(bc, p);
		const vec3 pca = closestPoint(ca, p);
		const real dab = distanceSquared(p, pab);
		const real dbc = distanceSquared(p, pbc);
		const real dca = distanceSquared(p, pca);
		const real dm = min(min(dab, dbc), dca);
		if (dm == dab)
			return pab;
		if (dm == dbc)
			return pbc;
		return pca;
	}

	vec3 closestPoint(const vec3 &pt, const Plane &pl)
	{
		CAGE_ASSERT(pl.normalized());
		return pt - dot(pl.normal, pt - pl.origin()) * pl.normal;
	}

	vec3 closestPoint(const vec3 &a, const Sphere &b)
	{
		if (distanceSquared(a, b.center) > sqr(b.radius))
			return normalize(a - b.center) * b.radius + b.center;
		return a;
	}

	vec3 closestPoint(const vec3 &a, const Aabb &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	vec3 closestPoint(const vec3 &a, const Cone &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	vec3 closestPoint(const vec3 &a, const Frustum &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}
}
