#include "math.h"

namespace cage
{
	vec2 vec2::parse(const string &str)
	{
		vec2 data;
		string s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 2; i++)
			data[i] = privat::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing vec2");
		return data;
	}

	vec3 vec3::parse(const string &str)
	{
		vec3 data;
		string s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 3; i++)
			data[i] = privat::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing vec3");
		return data;
	}

	vec4 vec4::parse(const string &str)
	{
		vec4 data;
		string s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 4; i++)
			data[i] = privat::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing vec4");
		return data;
	}

	quat quat::parse(const string &str)
	{
		quat data;
		string s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 4; i++)
			data[i] = privat::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing quat");
		return data;
	}

	quat::quat(rads pitch, rads yaw, rads roll)
	{
		vec3 eulerAngle = vec3(real(pitch), real(yaw), real(roll)) * 0.5;
		vec3 c;
		vec3 s;
		for (uint32 i = 0; i < 3; i++)
		{
			c[i] = cos(rads(eulerAngle[i]));
			s[i] = sin(rads(eulerAngle[i]));
		}
		data[0] = s[0] * c[1] * c[2] - c[0] * s[1] * s[2];
		data[1] = c[0] * s[1] * c[2] + s[0] * c[1] * s[2];
		data[2] = c[0] * c[1] * s[2] - s[0] * s[1] * c[2];
		data[3] = c[0] * c[1] * c[2] + s[0] * s[1] * s[2];
		*this = normalize(*this);
	}

	quat::quat(const vec3 &axis, rads angle)
	{
		vec3 vn = normalize(axis);
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

	vec3 operator * (const quat &l, const vec3 &r)
	{
		vec3 t = cross(vec3(l[0], l[1], l[2]), r) * 2;
		return r + t * l[3] + cross(vec3(l[0], l[1], l[2]), t);
	}

	vec3 operator * (const vec3 &l, const quat &r)
	{
		return r * l;
	}

	quat operator * (const quat &l, const quat &r)
	{
		return quat(l[3] * r[0] + l[0] * r[3] + l[1] * r[2] - l[2] * r[1],
			l[3] * r[1] + l[1] * r[3] + l[2] * r[0] - l[0] * r[2],
			l[3] * r[2] + l[2] * r[3] + l[0] * r[1] - l[1] * r[0],
			l[3] * r[3] - l[0] * r[0] - l[1] * r[1] - l[2] * r[2]);
	}

	quat lerp(const quat &a, const quat &b, real f)
	{
		return normalize(a * (1 - f) + b * f);
	}

	quat slerpPrecise(const quat &a, const quat &b, real f)
	{
		real dt = dot(a, b);
		quat c;
		if (dt < 0)
		{
			dt = -dt;
			c = -b;
		}
		else
			c = b;
		if (dt < 1 - 1e-7)
		{
			rads angle = acos(dt);
			return normalize((a * sin(angle * (1 - f)) + c * sin(angle * f)) * (1 / sin(angle)));
		}
		else
			return lerp(a, c, f);
	}

	quat slerp(const quat &l, const quat &r, real t)
	{
		// https://zeux.io/2016/05/05/optimizing-slerp/
		real ca = dot(l, r);
		real d = abs(ca);
		real A = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
		real B = 0.848013f + d * (-1.06021f + d * 0.215638f);
		real k = A * (t - 0.5f) * (t - 0.5f) + B;
		real ot = t + t * (t - 0.5f) * (t - 1) * k;
		return normalize(l * (1 - ot) + r * (ca > 0 ? ot : -ot));
	}

	quat rotate(const quat &from, const quat &toward, rads maxTurn)
	{
		CAGE_ASSERT(maxTurn >= degs(0), maxTurn);
		CAGE_ASSERT(maxTurn <= degs(180), maxTurn);
		real dt = abs(dot(from, toward));
		rads angle = acos(dt);
		if (angle > maxTurn)
			return slerp(from, toward, real(maxTurn / angle));
		else
			return toward;
	}

	void toAxisAngle(const quat &x, vec3 &axis, rads &angle)
	{
		angle = acos(x[3]) * 2;
		real s = sqrt(1 - x[3] * x[3]);
		if (s < 0.001)
			axis = vec3(1, 0, 0);
		else
		{
			s = 1 / s;
			axis[0] = x[0] * s;
			axis[1] = x[1] * s;
			axis[2] = x[2] * s;
		}
	}

	vec3 dominantAxis(const vec3 &x)
	{
		if (abs(x[0]) > abs(x[1]) && abs(x[0]) > abs(x[2]))
			return vec3(sign(x[0]), 0, 0);
		if (abs(x[1]) > abs(x[2]))
			return vec3(0, sign(x[1]), 0);
		return vec3(0, 0, sign(x[2]));
	}
}
