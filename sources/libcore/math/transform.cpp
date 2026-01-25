#include "math.h"

namespace cage
{
	namespace
	{
		uint32 findCommaIgnoreInsideParetheses(const String &s)
		{
			uint32 p = 0;
			for (uint32 i = 0; i < s.length(); i++)
			{
				switch (s[i])
				{
					case '(':
					{
						p++;
						break;
					}
					case ')':
					{
						if (p == 0)
							CAGE_THROW_ERROR(Exception, "mismatched ')'");
						p--;
						break;
					}
					case ',':
					{
						if (p == 0)
							return i;
						break;
					}
				}
			}
			return m;
		}
	}

	Transform Transform::parse(const String &str)
	{
		// ((x,y,z),(a,b,c,d),s)
		String s = str;
		if (s.length() < 21)
			CAGE_THROW_ERROR(Exception, "cannot parse transform: string too short");
		if (s[0] != '(' || s[s.length() - 1] != ')')
			CAGE_THROW_ERROR(Exception, "cannot parse transform: missing parentheses");
		s = subString(s, 1, s.length() - 2);
		const uint32 c1 = findCommaIgnoreInsideParetheses(s);
		const Vec3 p = Vec3::parse(subString(s, 0, c1));
		s = subString(s, c1 + 1, m);
		const uint32 c2 = findCommaIgnoreInsideParetheses(s);
		const Quat r = Quat::parse(subString(s, 0, c2));
		s = subString(s, c2 + 1, m);
		const Real sc = Real::parse(s);
		return Transform(p, r, sc);
	}

	bool Transform::valid() const
	{
		return orientation.valid() && position.valid() && scale.valid();
	}

	Transform operator*(Transform l, Transform r)
	{
		Transform res;
		res.orientation = l.orientation * r.orientation;
		res.scale = l.scale * r.scale;
		res.position = l.position + (r.position * l.orientation) * l.scale;
		return res;
	}

	Transform operator*(Transform l, Quat r)
	{
		Transform res = l;
		res.orientation = l.orientation * r; // not commutative
		return res;
	}

	Transform operator*(Quat l, Transform r)
	{
		Transform res = r;
		res.orientation = l * r.orientation; // not commutative
		return res;
	}

	Transform operator+(Transform l, Vec3 r)
	{
		Transform res = l;
		res.position += r;
		return res;
	}

	Transform operator+(Vec3 l, Transform r)
	{
		return r + l;
	}

	Transform operator*(Transform l, Real r)
	{
		Transform res = l;
		res.scale *= r;
		return res;
	}

	Transform operator*(Real l, Transform r)
	{
		return r * l;
	}

	Vec3 operator*(Transform l, Vec3 r)
	{
		return (l.orientation * r) * l.scale + l.position;
	}

	Vec3 operator*(Vec3 l, Transform r)
	{
		return r * l;
	}

	Transform inverse(Transform x)
	{
		Transform res;
		res.orientation = conjugate(x.orientation);
		res.scale = 1 / x.scale;
		res.position = -x.position * res.scale * res.orientation;
		return res;
	}
}
