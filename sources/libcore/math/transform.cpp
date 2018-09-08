#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include "math.h"

namespace cage
{
	transform::transform() : scale(1)
	{}

	transform::transform(const vec3 &position, const quat &orientation, real scale) : orientation(orientation), position(position), scale(scale)
	{}

	transform transform::operator * (const transform &other) const
	{
		transform r;
		r.orientation = orientation * other.orientation;
		r.scale = scale * other.scale;
		r.position = position + (other.position * orientation) * scale;
		return r;
	}

	transform quat::operator * (const transform &other) const
	{
		transform r(other);
		r.orientation = *this * other.orientation;
		return r;
	}

	transform transform::operator * (const quat &other) const
	{
		transform r(*this);
		r.orientation = orientation * other;
		return r;
	}

	transform transform::operator + (const vec3 &other) const
	{
		transform r(*this);
		r.position += other;
		return r;
	}

	transform transform::inverse() const
	{
		transform r;
		r.orientation = orientation.conjugate();
		r.scale = 1 / scale;
		r.position = -position * r.scale * r.orientation;
		return r;
	}

	transform interpolate(const transform &min, const transform &max, real value)
	{
		transform r;
		r.position = interpolate(min.position, max.position, value);
		r.orientation = interpolate(min.orientation, max.orientation, value);
		r.scale = interpolate(min.scale, max.scale, value);
		return r;
	}
}
