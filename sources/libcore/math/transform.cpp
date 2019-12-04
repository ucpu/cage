#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include "math.h"

namespace cage
{
	transform transform::parse(const string &str)
	{
		CAGE_THROW_CRITICAL(notImplemented, "transform::parse");
	}

	transform operator * (const transform &l, const transform &r)
	{
		transform res;
		res.orientation = l.orientation * r.orientation;
		res.scale = l.scale * r.scale;
		res.position = l.position + (r.position * l.orientation) * l.scale;
		return res;
	}

	transform operator * (const transform &l, const quat &r)
	{
		transform res = l;
		res.orientation = l.orientation * r; // not commutative
		return res;
	}

	transform operator * (const quat &l, const transform &r)
	{
		transform res = r;
		res.orientation = l * r.orientation; // not commutative
		return res;
	}

	transform operator + (const transform &l, const vec3 &r)
	{
		transform res = l;
		res.position += r;
		return res;
	}

	transform operator + (const vec3 &l, const transform &r)
	{
		return r + l;
	}

	transform operator * (const transform &l, const real &r)
	{
		transform res = l;
		res.scale *= r;
		return res;
	}

	transform operator * (const real &l, const transform &r)
	{
		return r * l;
	}

	vec3 operator * (const transform &l, const vec3 &r)
	{
		return (l.orientation * r) * l.scale + l.position;
	}

	vec3 operator * (const vec3 &l, const transform &r)
	{
		return r * l;
	}

	transform inverse(const transform &x)
	{
		transform res;
		res.orientation = conjugate(x.orientation);
		res.scale = 1 / x.scale;
		res.position = -x.position * res.scale * res.orientation;
		return res;
	}
}
