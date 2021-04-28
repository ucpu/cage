#include <cage-core/geometry.h>

#include <vector>

namespace cage
{
	// https://www.flipcode.com/archives/Smallest_Enclosing_Spheres.shtml modified

	namespace
	{
		Sphere MbSphere(const vec3 &O)
		{
			return Sphere(O, 1e-5);
		}

		Sphere MbSphere(const vec3 &O, const vec3 &A)
		{
			vec3 a = A - O;
			vec3 o = a * 0.5;
			return Sphere(O + o, length(o) + 1e-5);
		}

		Sphere MbSphere(const vec3 &O, const vec3 &A, const vec3 &B)
		{
			vec3 a = A - O;
			vec3 b = B - O;
			real denom = 2 * dot(cross(a, b), cross(a, b));
			vec3 o = (dot(b, b) * cross(cross(a, b), a) + dot(a, a) * cross(b, cross(a, b))) / denom;
			return Sphere(O + o, length(o) + 1e-5);
		}

		Sphere MbSphere(const vec3 &O, const vec3 &A, const vec3 &B, const vec3 &C)
		{
			vec3 a = A - O;
			vec3 b = B - O;
			vec3 c = C - O;
			real denom = 2 * determinant(mat3(a[0], a[1], a[2], b[0], b[1], b[2], c[0], c[1], c[2]));
			vec3 o = (dot(c, c) * cross(a, b) + dot(b, b) * cross(c, a) + dot(a, a) * cross(b, c)) / denom;
			return Sphere(O + o, length(o) + 1e-5);
		}

		real d2(const Sphere &s, const vec3 &p)
		{
			return distanceSquared(p, s.center) - sqr(s.radius);
		}

		Sphere recurseMini(const vec3 *P[], uint32 p, uint32 b)
		{
			Sphere MB = Sphere(vec3(), -1);
			switch (b)
			{
			case 1: MB = MbSphere(*P[-1]); break;
			case 2: MB = MbSphere(*P[-1], *P[-2]); break;
			case 3: MB = MbSphere(*P[-1], *P[-2], *P[-3]); break;
			case 4: return MbSphere(*P[-1], *P[-2], *P[-3], *P[-4]);
			}
			for (uint32 i = 0; i < p; i++)
			{
				if (d2(MB, *P[i]) > 0)
				{
					for (uint32 j = i; j > 0; j--)
					{
						const vec3 *T = P[j];
						P[j] = P[j - 1];
						P[j - 1] = T;
					}
					MB = recurseMini(P + 1, i, b + 1);
				}
			}
			return MB;
		}
	}

	Sphere::Sphere(const Frustum &other)
	{
		const Frustum::Corners corners = other.corners();
		const vec3 *L[8];
		int i = 0;
		for (const vec3 &v : corners.data)
			L[i++] = &v;
		*this = recurseMini(L, 8, 0);
	}

	Sphere makeSphere(PointerRange<const vec3> points)
	{
		switch (points.size())
		{
		case 0: return Sphere();
		case 1: return Sphere(points[0], 0);
		case 2: return Sphere(makeSegment(points[0], points[1]));
		case 3: return Sphere(Triangle(points[0], points[1], points[2]));
		}
		std::vector<const vec3 *> L;
		L.resize(points.size());
		int i = 0;
		for (const vec3 &v : points)
			L[i++] = &v;
		return recurseMini(L.data(), numeric_cast<uint32>(points.size()), 0);
	}
}
