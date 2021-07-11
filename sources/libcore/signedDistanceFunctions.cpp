#include <cage-core/geometry.h>
#include <cage-core/signedDistanceFunctions.h>

// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

namespace cage
{
	real sdfPlane(const vec3 &pos, const Plane &pln)
	{
		CAGE_ASSERT(pln.valid());
		const vec3 c = pln.normal * pln.d;
		return dot(pln.normal, pos - c);
	}

	real sdfSphere(const vec3 &pos, real radius)
	{
		return length(pos) - radius;
	}

	real sdfCapsule(const vec3 &pos, real prolong, real radius)
	{
		vec3 p = pos;
		p[2] += prolong * 0.5;
		p[2] -= clamp(p[2], 0, prolong);
		return length(p) - radius;
	}

	real sdfCylinder(const vec3 &pos, real halfHeight, real radius)
	{
		const vec2 d = abs(vec2(length(vec2(pos)), pos[2])) - vec2(radius, halfHeight);
		return min(max(d[0], d[1]), 0) + length(max(d, 0));
	}

	real sdfBox(const vec3 &pos, const vec3 &radius)
	{
		const vec3 p = abs(pos) - radius;
		return length(max(p, 0)) + min(max(p[0], max(p[1], p[2])), 0);
	}

	real sdfTetrahedron(const vec3 &pos, real radius)
	{
		constexpr const vec3 corners[4] = { vec3(1,1,1), vec3(1,-1,-1), vec3(-1,1,-1), vec3(-1,-1,1) };
		constexpr const Triangle tris[4] = {
			Triangle(corners[1], corners[3], corners[2]),
			Triangle(corners[0], corners[2], corners[3]),
			Triangle(corners[0], corners[3], corners[1]),
			Triangle(corners[0], corners[1], corners[2]),
		};

		const vec3 p = pos / radius;
		real mad = -real::Infinity();
		for (uint32 i = 0; i < 4; i++)
		{
			const Triangle &t = tris[i];
			vec3 k = closestPoint(Plane(t), p);
			real d = distance(t, p) * sign(dot(t.normal(), p - k));
			mad = max(mad, d);
		}
		return mad * radius;
	}

	real sdfOctahedron(const vec3 &pos, real radius)
	{
		const vec3 p = abs(pos);
		const real m = p[0] + p[1] + p[2] - radius;
		vec3 q;
		if (3.0 * p[0] < m)
			q = p;
		else if (3.0 * p[1] < m)
			q = vec3(p[1], p[2], p[0]);
		else if (3.0 * p[2] < m)
			q = vec3(p[2], p[0], p[1]);
		else
			return m * 0.57735027;
		const real k = clamp(0.5 * (q[2] - q[1] + radius), 0.0, radius);
		return length(vec3(q[0], q[1] - radius + k, q[2] - k));
	}

	real sdfHexagonalPrism(const vec3 &pos, real halfHeight, real radius)
	{
		constexpr vec3 k = vec3(-0.8660254, 0.5, 0.57735);
		vec3 p = abs(pos);
		p -= vec3(2.0 * min(dot(vec2(k), vec2(p)), 0.0) * vec2(k), 0);
		const vec2 d = vec2(length(vec2(p) - vec2(clamp(p[0], -k[2] * radius, k[2] * radius), radius)) * sign(p[1] - radius), p[2] - halfHeight);
		return min(max(d[0], d[1]), 0.0) + length(max(d, 0.0));
	}

	real sdfTorus(const vec3 &pos, real major, real minor)
	{
		const vec2 q = vec2(length(vec2(pos)) - major, pos[2]);
		return length(q) - minor;
	}

	real sdfKnot(const vec3 &pos, real scale, real k)
	{
		constexpr real TAU = real::Pi() * 2;
		constexpr real norm = 2 / 14.4;
		scale *= norm;
		vec3 p = pos / scale;
		const real r = length(vec2(p));
		rads a = atan2(p[0], p[1]);
		const rads oa = k * a;
		a = (a % (0.001 * TAU)) - 0.001 * TAU / 2;
		p[0] = r * cos(a) - 5;
		p[1] = r * sin(a);
		vec2 p2 = cos(oa) * vec2(p[0], p[2]) + sin(oa) * vec2(-p[2], p[0]);
		p[0] = abs(p2[0]) - 1.5;
		p[2] = p2[1];
		return (length(p) - 0.7) * scale;
	}
}
