#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include "math.h"

namespace cage
{
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
		res.orientation = l.orientation * r;
		return res;
	}

	transform operator + (const transform &l, const vec3 &r)
	{
		transform res = l;
		res.position += r;
		return res;
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
