#include <cage-core/geometry.h>

namespace cage
{
	// https://gist.github.com/jflipts/fc68d4eeacfcc04fbdb2bf38e0911850 modified

	namespace
	{
		void findMinMax(Real x0, Real x1, Real x2, Real &min, Real &max)
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

		bool planeBoxOverlap(Vec3 normal, Vec3 vert, Vec3 maxbox)
		{
			Vec3 vmin, vmax;
			Real v;
			for (uint32 q = 0; q < 3; q++)
			{
				v = vert[q];
				if (normal[q] > 0)
				{
					vmin[q] = -maxbox[q] - v;
					vmax[q] = maxbox[q] - v;
				}
				else
				{
					vmin[q] = maxbox[q] - v;
					vmax[q] = -maxbox[q] - v;
				}
			}
			if (dot(normal, vmin) > 0)
				return false;
			if (dot(normal, vmax) >= 0)
				return true;

			return false;
		}

		bool axisTestX01(Real a, Real b, Real fa, Real fb, const Vec3 &v0, const Vec3 &v2, const Vec3 &boxhalfsize, Real &rad, Real &min, Real &max, Real &p0, Real &p2)
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

		bool axisTestX2(Real a, Real b, Real fa, Real fb, const Vec3 &v0, const Vec3 &v1, const Vec3 &boxhalfsize, Real &rad, Real &min, Real &max, Real &p0, Real &p1)
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

		bool axisTestY02(Real a, Real b, Real fa, Real fb, const Vec3 &v0, const Vec3 &v2, const Vec3 &boxhalfsize, Real &rad, Real &min, Real &max, Real &p0, Real &p2)
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

		bool axisTestY1(Real a, Real b, Real fa, Real fb, const Vec3 &v0, const Vec3 &v1, const Vec3 &boxhalfsize, Real &rad, Real &min, Real &max, Real &p0, Real &p1)
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

		bool axisTestZ12(Real a, Real b, Real fa, Real fb, const Vec3 &v1, const Vec3 &v2, const Vec3 &boxhalfsize, Real &rad, Real &min, Real &max, Real &p1, Real &p2)
		{
			p1 = a * v1[0] - b * v1[1];
			p2 = a * v2[0] - b * v2[1];
			if (p1 < p2)
			{
				min = p1;
				max = p2;
			}
			else
			{
				min = p2;
				max = p1;
			}
			rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool axisTestZ0(Real a, Real b, Real fa, Real fb, const Vec3 &v0, const Vec3 &v1, const Vec3 &boxhalfsize, Real &rad, Real &min, Real &max, Real &p0, Real &p1)
		{
			p0 = a * v0[0] - b * v0[1];
			p1 = a * v1[0] - b * v1[1];
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
			rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];
			if (min > rad || max < -rad)
				return false;
			return true;
		}

		bool triBoxOverlap(const Vec3 &boxcenter, const Vec3 &boxhalfsize, const Vec3 &tv0, const Vec3 &tv1, const Vec3 &tv2)
		{
			Vec3 v0, v1, v2;
			Real min, max, p0, p1, p2, rad, fex, fey, fez;
			Vec3 normal, e0, e1, e2;
			v0 = tv0 - boxcenter;
			v1 = tv1 - boxcenter;
			v2 = tv2 - boxcenter;
			e0 = v1 - v0;
			e1 = v2 - v1;
			e2 = v0 - v2;
			fex = abs(e0[0]);
			fey = abs(e0[1]);
			fez = abs(e0[2]);
			if (!axisTestX01(e0[2], e0[1], fez, fey, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestY02(e0[2], e0[0], fez, fex, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestZ12(e0[1], e0[0], fey, fex, v1, v2, boxhalfsize, rad, min, max, p1, p2))
				return false;
			fex = abs(e1[0]);
			fey = abs(e1[1]);
			fez = abs(e1[2]);
			if (!axisTestX01(e1[2], e1[1], fez, fey, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestY02(e1[2], e1[0], fez, fex, v0, v2, boxhalfsize, rad, min, max, p0, p2))
				return false;
			if (!axisTestZ0(e1[1], e1[0], fey, fex, v0, v1, boxhalfsize, rad, min, max, p0, p1))
				return false;
			fex = abs(e2[0]);
			fey = abs(e2[1]);
			fez = abs(e2[2]);
			if (!axisTestX2(e2[2], e2[1], fez, fey, v0, v1, boxhalfsize, rad, min, max, p0, p1))
				return false;
			if (!axisTestY1(e2[2], e2[0], fez, fex, v0, v1, boxhalfsize, rad, min, max, p0, p1))
				return false;
			if (!axisTestZ12(e2[1], e2[0], fey, fex, v1, v2, boxhalfsize, rad, min, max, p1, p2))
				return false;
			findMinMax(v0[0], v1[0], v2[0], min, max);
			if (min > boxhalfsize[0] || max < -boxhalfsize[0])
				return false;
			findMinMax(v0[1], v1[1], v2[1], min, max);
			if (min > boxhalfsize[1] || max < -boxhalfsize[1])
				return false;
			findMinMax(v0[2], v1[2], v2[2], min, max);
			if (min > boxhalfsize[2] || max < -boxhalfsize[2])
				return false;
			normal = cross(e0, e1);
			if (!planeBoxOverlap(normal, v0, boxhalfsize))
				return false;
			return true;
		}
	}

	bool intersects(const Triangle &a, const Aabb &b)
	{
		return triBoxOverlap(b.center(), b.size() * 0.5, a[0], a[1], a[2]);
	}
}
