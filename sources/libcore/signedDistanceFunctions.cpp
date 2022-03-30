#include <cage-core/geometry.h>
#include <cage-core/signedDistanceFunctions.h>

// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

namespace cage
{
	Real sdfPlane(const Vec3 &pos, const Plane &pln)
	{
		CAGE_ASSERT(pln.valid());
		const Vec3 c = pln.normal * pln.d;
		return dot(pln.normal, pos - c);
	}

	Real sdfSphere(const Vec3 &pos, Real radius)
	{
		return length(pos) - radius;
	}

	Real sdfCapsule(const Vec3 &pos, Real prolong, Real radius)
	{
		Vec3 p = pos;
		p[2] += prolong * 0.5;
		p[2] -= clamp(p[2], 0, prolong);
		return length(p) - radius;
	}

	Real sdfCylinder(const Vec3 &pos, Real halfHeight, Real radius)
	{
		const Vec2 d = abs(Vec2(length(Vec2(pos)), pos[2])) - Vec2(radius, halfHeight);
		return min(max(d[0], d[1]), 0) + length(max(d, 0));
	}

	Real sdfBox(const Vec3 &pos, const Vec3 &radius)
	{
		const Vec3 p = abs(pos) - radius;
		return length(max(p, 0)) + min(max(p[0], max(p[1], p[2])), 0);
	}

	Real sdfTetrahedron(const Vec3 &pos, Real radius)
	{
		static constexpr const Vec3 corners[4] = { Vec3(1,1,1), Vec3(1,-1,-1), Vec3(-1,1,-1), Vec3(-1,-1,1) };
		static constexpr const Triangle tris[4] = {
			Triangle(corners[1], corners[3], corners[2]),
			Triangle(corners[0], corners[2], corners[3]),
			Triangle(corners[0], corners[3], corners[1]),
			Triangle(corners[0], corners[1], corners[2]),
		};

		const Vec3 p = pos / radius;
		Real mad = -Real::Infinity();
		for (uint32 i = 0; i < 4; i++)
		{
			const Triangle &t = tris[i];
			Vec3 k = closestPoint(Plane(t), p);
			Real d = distance(t, p) * sign(dot(t.normal(), p - k));
			mad = max(mad, d);
		}
		return mad * radius;
	}

	Real sdfOctahedron(const Vec3 &pos, Real radius)
	{
		const Vec3 p = abs(pos);
		const Real m = p[0] + p[1] + p[2] - radius;
		Vec3 q;
		if (3.0 * p[0] < m)
			q = p;
		else if (3.0 * p[1] < m)
			q = Vec3(p[1], p[2], p[0]);
		else if (3.0 * p[2] < m)
			q = Vec3(p[2], p[0], p[1]);
		else
			return m * 0.57735027;
		const Real k = clamp(0.5 * (q[2] - q[1] + radius), 0.0, radius);
		return length(Vec3(q[0], q[1] - radius + k, q[2] - k));
	}

	Real sdfHexagonalPrism(const Vec3 &pos, Real halfHeight, Real radius)
	{
		static constexpr Vec3 k = Vec3(-0.8660254, 0.5, 0.57735);
		Vec3 p = abs(pos);
		p -= Vec3(2.0 * min(dot(Vec2(k), Vec2(p)), 0.0) * Vec2(k), 0);
		const Vec2 d = Vec2(length(Vec2(p) - Vec2(clamp(p[0], -k[2] * radius, k[2] * radius), radius)) * sign(p[1] - radius), p[2] - halfHeight);
		return min(max(d[0], d[1]), 0.0) + length(max(d, 0.0));
	}

	Real sdfTorus(const Vec3 &pos, Real major, Real minor)
	{
		const Vec2 q = Vec2(length(Vec2(pos)) - major, pos[2]);
		return length(q) - minor;
	}

	Real sdfKnot(const Vec3 &pos, Real scale, Real k)
	{
		static constexpr Real TAU = Real::Pi() * 2;
		static constexpr Real norm = 2 / 14.4;
		scale *= norm;
		Vec3 p = pos / scale;
		const Real r = length(Vec2(p));
		Rads a = atan2(p[0], p[1]);
		const Rads oa = k * a;
		a = (a % (0.001 * TAU)) - 0.001 * TAU / 2;
		p[0] = r * cos(a) - 5;
		p[1] = r * sin(a);
		Vec2 p2 = cos(oa) * Vec2(p[0], p[2]) + sin(oa) * Vec2(-p[2], p[0]);
		p[0] = abs(p2[0]) - 1.5;
		p[2] = p2[1];
		return (length(p) - 0.7) * scale;
	}
}
