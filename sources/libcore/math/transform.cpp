#include "math.h"

namespace cage
{
	Transform Transform::parse(const String &str)
	{
		CAGE_THROW_CRITICAL(Exception, "transform::parse");
	}

	Transform operator*(const Transform &l, const Transform &r) noexcept
	{
		Transform res;
		res.orientation = l.orientation * r.orientation;
		res.scale = l.scale * r.scale;
		res.position = l.position + (r.position * l.orientation) * l.scale;
		return res;
	}

	Transform operator*(const Transform &l, const Quat &r) noexcept
	{
		Transform res = l;
		res.orientation = l.orientation * r; // not commutative
		return res;
	}

	Transform operator*(const Quat &l, const Transform &r) noexcept
	{
		Transform res = r;
		res.orientation = l * r.orientation; // not commutative
		return res;
	}

	Transform operator+(const Transform &l, const Vec3 &r) noexcept
	{
		Transform res = l;
		res.position += r;
		return res;
	}

	Transform operator+(const Vec3 &l, const Transform &r) noexcept
	{
		return r + l;
	}

	Transform operator*(const Transform &l, const Real &r) noexcept
	{
		Transform res = l;
		res.scale *= r;
		return res;
	}

	Transform operator*(const Real &l, const Transform &r) noexcept
	{
		return r * l;
	}

	Vec3 operator*(const Transform &l, const Vec3 &r) noexcept
	{
		return (l.orientation * r) * l.scale + l.position;
	}

	Vec3 operator*(const Vec3 &l, const Transform &r) noexcept
	{
		return r * l;
	}

	Transform inverse(const Transform &x)
	{
		Transform res;
		res.orientation = conjugate(x.orientation);
		res.scale = 1 / x.scale;
		res.position = -x.position * res.scale * res.orientation;
		return res;
	}
}
