#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include "math.h"

namespace cage
{
	vec2 vec2::parse(const string &str)
	{
		vec2 data;
		string s = detail::tryRemoveParentheses(str);
		for (uint32 i = 0; i < Dimension; i++)
			data[i] = detail::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(exception, "error parsing vec2");
		return data;
	}

	vec3 vec3::parse(const string &str)
	{
		vec3 data;
		string s = detail::tryRemoveParentheses(str);
		for (uint32 i = 0; i < Dimension; i++)
			data[i] = detail::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(exception, "error parsing vec3");
		return data;
	}

	vec4 vec4::parse(const string &str)
	{
		vec4 data;
		string s = detail::tryRemoveParentheses(str);
		for (uint32 i = 0; i < Dimension; i++)
			data[i] = detail::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(exception, "error parsing vec4");
		return data;
	}

	quat quat::parse(const string &str)
	{
		quat data;
		string s = detail::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 4; i++)
			data[i] = detail::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(exception, "error parsing quat");
		return data;
	}

	quat::quat()
	{
		data[0] = data[1] = data[2] = 0;
		data[3] = 1;
	}

	quat::quat(real x, real y, real z, real w)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}

	quat::quat(rads pitch, rads yaw, rads roll)
	{
		vec3 eulerAngle = vec3(real(pitch), real(yaw), real(roll)) * real(0.5);
		vec3 c;
		vec3 s;
		for (uint32 i = 0; i < 3; i++)
		{
			c[i] = cos(rads(eulerAngle[i]));
			s[i] = sin(rads(eulerAngle[i]));
		}
		this->data[0] = s.data[0] * c.data[1] * c.data[2] - c.data[0] * s.data[1] * s.data[2];
		this->data[1] = c.data[0] * s.data[1] * c.data[2] + s.data[0] * c.data[1] * s.data[2];
		this->data[2] = c.data[0] * c.data[1] * s.data[2] - s.data[0] * s.data[1] * c.data[2];
		this->data[3] = c.data[0] * c.data[1] * c.data[2] + s.data[0] * s.data[1] * s.data[2];
		*this = this->normalize();
	}

	quat::quat(const vec3 &axis, rads angle)
	{
		vec3 vn = axis.normalize();
		real sinAngle = sin(angle * 0.5);
		data[0] = vn.data[0] * sinAngle;
		data[1] = vn.data[1] * sinAngle;
		data[2] = vn.data[2] * sinAngle;
		data[3] = cos(angle * 0.5);
	}

	quat::quat(const mat3 & rot)
	{
#define a(x,y) rot[y*3+x]
		real trace = a(0, 0) + a(1, 1) + a(2, 2);
		if (trace > 0)
		{
			real s = 0.5 / sqrt(trace + 1);
			data[3] = 0.25 / s;
			data[0] = (a(2, 1) - a(1, 2)) * s;
			data[1] = (a(0, 2) - a(2, 0)) * s;
			data[2] = (a(1, 0) - a(0, 1)) * s;
		}
		else if (a(0, 0) > a(1, 1) && a(0, 0) > a(2, 2))
		{
			real s = 2 * sqrt(1 + a(0, 0) - a(1, 1) - a(2, 2));
			data[3] = (a(2, 1) - a(1, 2)) / s;
			data[0] = 0.25 * s;
			data[1] = (a(0, 1) + a(1, 0)) / s;
			data[2] = (a(0, 2) + a(2, 0)) / s;
		}
		else if (a(1, 1) > a(2, 2))
		{
			real s = 2.0 * sqrt(1.0f + a(1, 1) - a(0, 0) - a(2, 2));
			data[3] = (a(0, 2) - a(2, 0)) / s;
			data[0] = (a(0, 1) + a(1, 0)) / s;
			data[1] = 0.25 * s;
			data[2] = (a(1, 2) + a(2, 1)) / s;
		}
		else
		{
			real s = 2.0 * sqrt(1.0f + a(2, 2) - a(0, 0) - a(1, 1));
			data[3] = (a(1, 0) - a(0, 1)) / s;
			data[0] = (a(0, 2) + a(2, 0)) / s;
			data[1] = (a(1, 2) + a(2, 1)) / s;
			data[2] = 0.25 * s;
		}
#undef a
	}

	quat::quat(const vec3 &forward, const vec3 &up, bool keepUp)
	{
		*this = quat(mat3(forward, up, keepUp));
	}

	vec3 quat::operator * (const vec3 &other) const
	{
		vec3 t = cross(vec3(data[0], data[1], data[2]), other) * 2;
		return other + t * data[3] + cross(vec3(data[0], data[1], data[2]), t);
	}

	quat quat::operator * (const quat &other) const
	{
		return quat(data[3] * other.data[0] + data[0] * other.data[3] + data[1] * other.data[2] - data[2] * other.data[1],
			data[3] * other.data[1] + data[1] * other.data[3] + data[2] * other.data[0] - data[0] * other.data[2],
			data[3] * other.data[2] + data[2] * other.data[3] + data[0] * other.data[1] - data[1] * other.data[0],
			data[3] * other.data[3] - data[0] * other.data[0] - data[1] * other.data[1] - data[2] * other.data[2]);
	}

	quat lerp(const quat &min, const quat &max, real value)
	{
		return (min * (1 - value) + max * value).normalize();
	}

	quat slerpPrecise(const quat &min, const quat &max, real value)
	{
		real dot = min.dot(max);
		quat c;
		if (dot < 0)
		{
			dot = -dot;
			c = max.inverse();
		}
		else
			c = max;
		if (dot < 1 - 1e-7)
		{
			rads angle = aCos(dot);
			return ((min * sin(angle * (1 - value)) + c * sin(angle * value)) * (1 / sin(angle))).normalize();
		}
		else
			return lerp(min, c, value);
	}

	quat slerp(const quat &l, const quat &r, real t)
	{
		// https://zeux.io/2016/05/05/optimizing-slerp/
		real ca = l.dot(r);
		real d = abs(ca);
		real A = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
		real B = 0.848013f + d * (-1.06021f + d * 0.215638f);
		real k = A * (t - 0.5f) * (t - 0.5f) + B;
		real ot = t + t * (t - 0.5f) * (t - 1) * k;
		return normalize(l * (1 - ot) + r * (ca > 0 ? ot : -ot));
	}

	quat rotate(const quat &from, const quat &toward, rads maxTurn)
	{
		CAGE_ASSERT_RUNTIME(maxTurn >= degs(0), maxTurn);
		CAGE_ASSERT_RUNTIME(maxTurn <= degs(180), maxTurn);
		real dot = abs(from.dot(toward));
		rads angle = aCos(dot);
		if (angle > maxTurn)
			return slerp(from, toward, maxTurn / angle);
		else
			return toward;
	}

	void quat::toAxisAngle(vec3 &axis, rads &angle) const
	{
		angle = aCos(data[3]) * 2;
		real s = sqrt(1 - data[3] * data[3]);
		if (s < 0.001)
			axis = vec3(1, 0, 0);
		else
		{
			s = 1 / s;
			axis[0] = data[0] * s;
			axis[1] = data[1] * s;
			axis[2] = data[2] * s;
		}
	}

	vec3 vec3::primaryAxis() const
	{
		if (data[0].abs() > data[1].abs() && data[0].abs() > data[2].abs())
			return vec3(data[0].sign(), 0, 0);
		if (data[1].abs() > data[2].abs())
			return vec3(0, data[1].sign(), 0);
		return vec3(0, 0, data[2].sign());
	}

	vec2 vec2::Nan() { return vec2(real::Nan(), real::Nan()); }
	vec3 vec3::Nan() { return vec3(real::Nan(), real::Nan(), real::Nan()); }
	vec4 vec4::Nan() { return vec4(real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
	quat quat::Nan() { return quat(real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
}
