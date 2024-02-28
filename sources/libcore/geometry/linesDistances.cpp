#include <cage-core/geometry.h>

namespace cage
{
	namespace
	{
		Real distanceLines(const Vec3 a1, const Vec3 a2, const Vec3 b1, const Vec3 b2)
		{
			// algorithm taken from http://geomalgorithms.com/a07-_distance.html and modified

			// Copyright 2001 softSurfer, 2012 Dan Sunday
			// This code may be freely used, distributed and modified for any purpose
			// providing that this copyright notice is included with it.
			// SoftSurfer makes no warranty for this code, and cannot be held
			// liable for any real or imagined damage resulting from its use.
			// Users of this code must verify correctness for their application.

			Vec3 u = a2 - a1;
			Vec3 v = b2 - b1;
			Vec3 w = a1 - b1;
			Real a = dot(u, u);
			Real b = dot(u, v);
			Real c = dot(v, v);
			Real d = dot(u, w);
			Real e = dot(v, w);
			Real D = a * c - b * b;
			Real sc, tc;
			if (D < 1e-5)
			{
				sc = 0.0;
				tc = (b > c ? d / b : e / c);
			}
			else
			{
				sc = (b * e - c * d) / D;
				tc = (a * e - b * d) / D;
			}
			Vec3 dP = w + (sc * u) - (tc * v);
			return length(dP);
		}

		Real distanceSegments(const Vec3 a1, const Vec3 a2, const Vec3 b1, const Vec3 b2)
		{
			// algorithm taken from http://geomalgorithms.com/a07-_distance.html and modified

			// Copyright 2001 softSurfer, 2012 Dan Sunday
			// This code may be freely used, distributed and modified for any purpose
			// providing that this copyright notice is included with it.
			// SoftSurfer makes no warranty for this code, and cannot be held
			// liable for any real or imagined damage resulting from its use.
			// Users of this code must verify correctness for their application.

			Vec3 u = a2 - a1;
			Vec3 v = b2 - b1;
			Vec3 w = a1 - b1;
			Real a = dot(u, u);
			Real b = dot(u, v);
			Real c = dot(v, v);
			Real d = dot(u, w);
			Real e = dot(v, w);
			Real D = a * c - b * b;
			Real sc, sN, sD = D;
			Real tc, tN, tD = D;
			if (D < 1e-5)
			{
				sN = 0.0;
				sD = 1.0;
				tN = e;
				tD = c;
			}
			else
			{
				sN = (b * e - c * d);
				tN = (a * e - b * d);
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
				else
				{
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
				else
				{
					sN = (-d + b);
					sD = a;
				}
			}
			sc = (abs(sN) < 1e-5 ? 0.0 : sN / sD);
			tc = (abs(tN) < 1e-5 ? 0.0 : tN / tD);
			Vec3 dP = w + (sc * u) - (tc * v);
			return length(dP);
		}
	}

	Real distance(Line a, Line b)
	{
		if (a.isLine() && b.isLine())
			return distanceLines(a.origin, a.origin + a.direction, b.origin, b.origin + b.direction);
		if (a.isSegment() && b.isSegment())
			return distanceSegments(a.a(), a.b(), b.a(), b.b());
		if (a.isPoint())
			return distance(a.a(), b);
		if (b.isPoint())
			return distance(a, b.a());
		CAGE_THROW_CRITICAL(Exception, "not implemented distance(Line, Line) combination");
	}
}
