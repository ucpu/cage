#include "math.h"

namespace cage
{
	Vec2 Vec2::parse(const String &str)
	{
		Vec2 data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 2; i++)
			data[i] = toFloat(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing vec2");
		return data;
	}

	Vec3 Vec3::parse(const String &str)
	{
		Vec3 data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 3; i++)
			data[i] = toFloat(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing vec3");
		return data;
	}

	Vec4 Vec4::parse(const String &str)
	{
		Vec4 data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 4; i++)
			data[i] = toFloat(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing vec4");
		return data;
	}

	Vec2i Vec2i::parse(const String &str)
	{
		Vec2i data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 2; i++)
			data[i] = toSint32(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing ivec2");
		return data;
	}

	Vec3i Vec3i::parse(const String &str)
	{
		Vec3i data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 3; i++)
			data[i] = toSint32(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing ivec3");
		return data;
	}

	Vec4i Vec4i::parse(const String &str)
	{
		Vec4i data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 4; i++)
			data[i] = toSint32(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing ivec4");
		return data;
	}

	Quat Quat::parse(const String &str)
	{
		Quat data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 4; i++)
			data[i] = toFloat(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing quat");
		return data;
	}

	Quat::Quat(Rads pitch, Rads yaw, Rads roll)
	{
		Vec3 eulerAngle = Vec3(Real(pitch), Real(yaw), Real(roll)) * 0.5;
		Vec3 c;
		Vec3 s;
		for (uint32 i = 0; i < 3; i++)
		{
			c[i] = cos(Rads(eulerAngle[i]));
			s[i] = sin(Rads(eulerAngle[i]));
		}
		data[0] = s[0] * c[1] * c[2] - c[0] * s[1] * s[2];
		data[1] = c[0] * s[1] * c[2] + s[0] * c[1] * s[2];
		data[2] = c[0] * c[1] * s[2] - s[0] * s[1] * c[2];
		data[3] = c[0] * c[1] * c[2] + s[0] * s[1] * s[2];
		*this = normalize(*this);
	}

	Quat::Quat(const Vec3 &axis, Rads angle)
	{
		Vec3 vn = normalize(axis);
		Real sinAngle = sin(angle * 0.5);
		data[0] = vn.data[0] * sinAngle;
		data[1] = vn.data[1] * sinAngle;
		data[2] = vn.data[2] * sinAngle;
		data[3] = cos(angle * 0.5);
	}

	Quat::Quat(const Mat3 & rot)
	{
#define a(x,y) rot[y*3+x]
		Real trace = a(0, 0) + a(1, 1) + a(2, 2);
		if (trace > 0)
		{
			Real s = 0.5 / sqrt(trace + 1);
			data[3] = 0.25 / s;
			data[0] = (a(2, 1) - a(1, 2)) * s;
			data[1] = (a(0, 2) - a(2, 0)) * s;
			data[2] = (a(1, 0) - a(0, 1)) * s;
		}
		else if (a(0, 0) > a(1, 1) && a(0, 0) > a(2, 2))
		{
			Real s = 2 * sqrt(1 + a(0, 0) - a(1, 1) - a(2, 2));
			data[3] = (a(2, 1) - a(1, 2)) / s;
			data[0] = 0.25 * s;
			data[1] = (a(0, 1) + a(1, 0)) / s;
			data[2] = (a(0, 2) + a(2, 0)) / s;
		}
		else if (a(1, 1) > a(2, 2))
		{
			Real s = 2.0 * sqrt(1.0f + a(1, 1) - a(0, 0) - a(2, 2));
			data[3] = (a(0, 2) - a(2, 0)) / s;
			data[0] = (a(0, 1) + a(1, 0)) / s;
			data[1] = 0.25 * s;
			data[2] = (a(1, 2) + a(2, 1)) / s;
		}
		else
		{
			Real s = 2.0 * sqrt(1.0f + a(2, 2) - a(0, 0) - a(1, 1));
			data[3] = (a(1, 0) - a(0, 1)) / s;
			data[0] = (a(0, 2) + a(2, 0)) / s;
			data[1] = (a(1, 2) + a(2, 1)) / s;
			data[2] = 0.25 * s;
		}
#undef a
	}

	Quat::Quat(const Vec3 &forward, const Vec3 &up, bool keepUp)
	{
		*this = Quat(Mat3(forward, up, keepUp));
	}

	Vec3 operator * (const Quat &l, const Vec3 &r) noexcept
	{
		Vec3 t = cross(Vec3(l[0], l[1], l[2]), r) * 2;
		return r + t * l[3] + cross(Vec3(l[0], l[1], l[2]), t);
	}

	Vec3 operator * (const Vec3 &l, const Quat &r) noexcept
	{
		return r * l;
	}

	Quat lerp(const Quat &a, const Quat &b, Real f)
	{
		return normalize(a * (1 - f) + b * f);
	}

	Quat slerpPrecise(const Quat &a, const Quat &b, Real f)
	{
		Real dt = dot(a, b);
		Quat c;
		if (dt < 0)
		{
			dt = -dt;
			c = -b;
		}
		else
			c = b;
		if (dt < 1 - 1e-7)
		{
			Rads angle = acos(dt);
			return normalize((a * sin(angle * (1 - f)) + c * sin(angle * f)) * (1 / sin(angle)));
		}
		else
			return lerp(a, c, f);
	}

	Quat slerp(const Quat &l, const Quat &r, Real t)
	{
		// https://zeux.io/2016/05/05/optimizing-slerp/
		Real ca = dot(l, r);
		Real d = abs(ca);
		Real A = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
		Real B = 0.848013f + d * (-1.06021f + d * 0.215638f);
		Real k = A * (t - 0.5f) * (t - 0.5f) + B;
		Real ot = t + t * (t - 0.5f) * (t - 1) * k;
		return normalize(l * (1 - ot) + r * (ca > 0 ? ot : -ot));
	}

	Quat rotate(const Quat &from, const Quat &toward, Rads maxTurn)
	{
		CAGE_ASSERT(maxTurn >= Degs(0));
		CAGE_ASSERT(maxTurn <= Degs(180));
		Real dt = abs(dot(from, toward));
		Rads angle = acos(dt);
		if (angle > maxTurn)
			return slerp(from, toward, Real(maxTurn / angle));
		else
			return toward;
	}

	Rads angle(const Quat &a, const Quat &b)
	{
		Quat q = conjugate(a) * b;
		Vec3 v = Vec3(q.data[0], q.data[1], q.data[2]);
		return 2 * atan2(q.data[3], length(v));
	}

	Rads pitch(const Quat &q)
	{
		const Real qx = q[0];
		const Real qy = q[1];
		const Real qz = q[2];
		const Real qw = q[3];
		return atan2(sqr(qw) - sqr(qx) - sqr(qy) + sqr(qz), 2 * (qy * qz + qw * qx));
	}

	Rads yaw(const Quat &q)
	{
		const Real qx = q[0];
		const Real qy = q[1];
		const Real qz = q[2];
		const Real qw = q[3];
		return asin(clamp(-2 * (qx * qz - qw * qy), -1, 1));
	}

	Rads roll(const Quat &q)
	{
		const Real qx = q[0];
		const Real qy = q[1];
		const Real qz = q[2];
		const Real qw = q[3];
		return atan2(sqr(qw) + sqr(qx) - sqr(qy) - sqr(qz), 2 * (qx * qy + qw * qz));
	}

	void toAxisAngle(const Quat &x, Vec3 &axis, Rads &angle)
	{
		angle = acos(clamp(x[3], -1, 1)) * 2;
		Real s = sqrt(1 - x[3] * x[3]);
		if (s < 0.001)
			axis = Vec3(1, 0, 0);
		else
		{
			s = 1 / s;
			axis[0] = x[0] * s;
			axis[1] = x[1] * s;
			axis[2] = x[2] * s;
		}
	}

	Vec3 dominantAxis(const Vec3 &x)
	{
		if (abs(x[0]) > abs(x[1]) && abs(x[0]) > abs(x[2]))
			return Vec3(sign(x[0]), 0, 0);
		if (abs(x[1]) > abs(x[2]))
			return Vec3(0, sign(x[1]), 0);
		return Vec3(0, 0, sign(x[2]));
	}
}
