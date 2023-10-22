#include "math.h"

namespace cage
{
	Transform Transform::parse(const String &str)
	{
		CAGE_THROW_CRITICAL(Exception, "transform::parse");
	}

	bool Transform::valid() const noexcept
	{
		return orientation.valid() && position.valid() && scale.valid();
	}

	Transform operator*(Transform l, Transform r) noexcept
	{
		Transform res;
		res.orientation = l.orientation * r.orientation;
		res.scale = l.scale * r.scale;
		res.position = l.position + (r.position * l.orientation) * l.scale;
		return res;
	}

	Transform operator*(Transform l, Quat r) noexcept
	{
		Transform res = l;
		res.orientation = l.orientation * r; // not commutative
		return res;
	}

	Transform operator*(Quat l, Transform r) noexcept
	{
		Transform res = r;
		res.orientation = l * r.orientation; // not commutative
		return res;
	}

	Transform operator+(Transform l, Vec3 r) noexcept
	{
		Transform res = l;
		res.position += r;
		return res;
	}

	Transform operator+(Vec3 l, Transform r) noexcept
	{
		return r + l;
	}

	Transform operator*(Transform l, Real r) noexcept
	{
		Transform res = l;
		res.scale *= r;
		return res;
	}

	Transform operator*(Real l, Transform r) noexcept
	{
		return r * l;
	}

	Vec3 operator*(Transform l, Vec3 r) noexcept
	{
		return (l.orientation * r) * l.scale + l.position;
	}

	Vec3 operator*(Vec3 l, Transform r) noexcept
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
