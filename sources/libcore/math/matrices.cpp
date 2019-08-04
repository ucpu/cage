#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include "math.h"

namespace cage
{
	mat3 mat3::parse(const string &str)
	{
		mat3 data;
		string s = detail::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 9; i++)
			data[i] = detail::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(exception, "error parsing mat3");
		return data;
	}

	mat3::mat3(const quat &other)
	{
		real x2 = other[0] * other[0];
		real y2 = other[1] * other[1];
		real z2 = other[2] * other[2];
		real xy = other[0] * other[1];
		real xz = other[0] * other[2];
		real yz = other[1] * other[2];
		real wx = other[3] * other[0];
		real wy = other[3] * other[1];
		real wz = other[3] * other[2];
		data[0] = (real)1.0 - (real)2.0 * (y2 + z2);
		data[1] = (real)2.0 * (xy + wz);
		data[2] = (real)2.0 * (xz - wy);
		data[3] = (real)2.0 * (xy - wz);
		data[4] = (real)1.0 - (real)2.0 * (x2 + z2);
		data[5] = (real)2.0 * (yz + wx);
		data[6] = (real)2.0 * (xz + wy);
		data[7] = (real)2.0 * (yz - wx);
		data[8] = (real)1.0 - (real)2.0 * (x2 + y2);
	}

	mat3::mat3(const vec3 &forward_, const vec3 &up_, bool keepUp)
	{
		vec3 forward = forward_;
		vec3 up = up_;
		vec3 right = cross(forward, up);
		if (keepUp)
			forward = cross(up, right);
		else
			up = cross(right, forward);
		forward = normalize(forward);
		up = normalize(up);
		right = cross(forward, up);
		*this = mat3(
			right[0], right[1], right[2],
			up[0], up[1], up[2],
			-forward[0], -forward[1], -forward[2]
		);
	}

	vec3 operator * (const mat3 &l, const vec3 &r)
	{
		vec3 res;
		for (uint8 i = 0; i < 3; i++)
		{
			for (uint8 j = 0; j < 3; j++)
				res[i] += r[j] * l[j * 3 + i];
		}
		return res;
	}

	vec3 operator * (const vec3 &l, const mat3 &r)
	{
		return transpose(r) * l;
	}

	mat3 operator * (const mat3 &l, const mat3 &r)
	{
		mat3 res = mat3::Zero();
		for (uint8 x = 0; x < 3; x++)
		{
			for (uint8 y = 0; y < 3; y++)
			{
				for (uint8 z = 0; z < 3; z++)
					res[y * 3 + x] += l[z * 3 + x] * r[y * 3 + z];
			}
		}
		return res;
	}

	mat3 transpose(const mat3 &x)
	{
		mat3 tmp;
		for (uint8 a = 0; a < 3; a++)
			for (uint8 b = 0; b < 3; b++)
				tmp[a * 3 + b] = x[b * 3 + a];
		return tmp;
	}

	mat3 normalize(const mat3 &x)
	{
		real len;
		for (uint8 i = 0; i < 9; i++)
			len += sqr(x[i]);
		len = sqrt(len);
		return x * (1 / len);
	}

	real determinant(const mat3 &x)
	{
		return
			x[0 * 3 + 0] * (x[1 * 3 + 1] * x[2 * 3 + 2] - x[2 * 3 + 1] * x[1 * 3 + 2])
			- x[0 * 3 + 1] * (x[1 * 3 + 0] * x[2 * 3 + 2] - x[1 * 3 + 2] * x[2 * 3 + 0])
			+ x[0 * 3 + 2] * (x[1 * 3 + 0] * x[2 * 3 + 1] - x[1 * 3 + 1] * x[2 * 3 + 0]);
	}

	mat3 inverse(const mat3 &x)
	{
		real invdet = 1 / determinant(x);
		mat3 res;
		res[0 * 3 + 0] = (x[1 * 3 + 1] * x[2 * 3 + 2] - x[2 * 3 + 1] * x[1 * 3 + 2]) * invdet;
		res[0 * 3 + 1] = -(x[0 * 3 + 1] * x[2 * 3 + 2] - x[0 * 3 + 2] * x[2 * 3 + 1]) * invdet;
		res[0 * 3 + 2] = (x[0 * 3 + 1] * x[1 * 3 + 2] - x[0 * 3 + 2] * x[1 * 3 + 1]) * invdet;
		res[1 * 3 + 0] = -(x[1 * 3 + 0] * x[2 * 3 + 2] - x[1 * 3 + 2] * x[2 * 3 + 0]) * invdet;
		res[1 * 3 + 1] = (x[0 * 3 + 0] * x[2 * 3 + 2] - x[0 * 3 + 2] * x[2 * 3 + 0]) * invdet;
		res[1 * 3 + 2] = -(x[0 * 3 + 0] * x[1 * 3 + 2] - x[1 * 3 + 0] * x[0 * 3 + 2]) * invdet;
		res[2 * 3 + 0] = (x[1 * 3 + 0] * x[2 * 3 + 1] - x[2 * 3 + 0] * x[1 * 3 + 1]) * invdet;
		res[2 * 3 + 1] = -(x[0 * 3 + 0] * x[2 * 3 + 1] - x[2 * 3 + 0] * x[0 * 3 + 1]) * invdet;
		res[2 * 3 + 2] = (x[0 * 3 + 0] * x[1 * 3 + 1] - x[1 * 3 + 0] * x[0 * 3 + 1]) * invdet;
		return res;
	}

	mat4 mat4::parse(const string &str)
	{
		mat4 data;
		string s = detail::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 16; i++)
			data[i] = detail::mathSplit(s).toFloat();
		if (!s.empty())
			CAGE_THROW_ERROR(exception, "error parsing mat4");
		return data;
	}

	mat4::mat4(const vec3 &p, const quat &q, const vec3 &s)
	{
		// this = T * R * S
		mat3 r(q);
		*this = mat4(
			r[0] * s[0], r[1] * s[0], r[2] * s[0], 0,
			r[3] * s[1], r[4] * s[1], r[5] * s[1], 0,
			r[6] * s[2], r[7] * s[2], r[8] * s[2], 0,
			p[0], p[1], p[2], 1
		);
	}

	vec4 operator * (const mat4 &l, const vec4 &r)
	{
		vec4 res;
		for (uint8 i = 0; i < 4; i++)
		{
			for (uint8 j = 0; j < 4; j++)
				res[i] += r[j] * l[j * 4 + i];
		}
		return res;
	}

	vec4 operator * (const vec4 &l, const mat4 &r)
	{
		return transpose(r) * l;
	}

	mat4 operator + (const mat4 &l, const mat4 &r)
	{
		mat4 res;
		for (uint8 i = 0; i < 16; i++)
			res[i] = l[i] + r[i];
		return res;
	}

	mat4 operator * (const mat4 &l, const mat4 &r)
	{
		mat4 res = mat4::Zero();
		for (uint8 x = 0; x < 4; x++)
		{
			for (uint8 y = 0; y < 4; y++)
			{
				for (uint8 z = 0; z < 4; z++)
					res[y * 4 + x] += l[z * 4 + x] * r[y * 4 + z];
			}
		}
		return res;
	}

	mat4 inverse(const mat4 &x)
	{
		mat4 res;
		res[0] = x[5] * x[10] * x[15] - x[5] * x[11] * x[14] - x[9] * x[6] * x[15] + x[9] * x[7] * x[14] + x[13] * x[6] * x[11] - x[13] * x[7] * x[10];
		res[4] = -x[4] * x[10] * x[15] + x[4] * x[11] * x[14] + x[8] * x[6] * x[15] - x[8] * x[7] * x[14] - x[12] * x[6] * x[11] + x[12] * x[7] * x[10];
		res[8] = x[4] * x[9] * x[15] - x[4] * x[11] * x[13] - x[8] * x[5] * x[15] + x[8] * x[7] * x[13] + x[12] * x[5] * x[11] - x[12] * x[7] * x[9];
		res[12] = -x[4] * x[9] * x[14] + x[4] * x[10] * x[13] + x[8] * x[5] * x[14] - x[8] * x[6] * x[13] - x[12] * x[5] * x[10] + x[12] * x[6] * x[9];
		res[1] = -x[1] * x[10] * x[15] + x[1] * x[11] * x[14] + x[9] * x[2] * x[15] - x[9] * x[3] * x[14] - x[13] * x[2] * x[11] + x[13] * x[3] * x[10];
		res[5] = x[0] * x[10] * x[15] - x[0] * x[11] * x[14] - x[8] * x[2] * x[15] + x[8] * x[3] * x[14] + x[12] * x[2] * x[11] - x[12] * x[3] * x[10];
		res[9] = -x[0] * x[9] * x[15] + x[0] * x[11] * x[13] + x[8] * x[1] * x[15] - x[8] * x[3] * x[13] - x[12] * x[1] * x[11] + x[12] * x[3] * x[9];
		res[13] = x[0] * x[9] * x[14] - x[0] * x[10] * x[13] - x[8] * x[1] * x[14] + x[8] * x[2] * x[13] + x[12] * x[1] * x[10] - x[12] * x[2] * x[9];
		res[2] = x[1] * x[6] * x[15] - x[1] * x[7] * x[14] - x[5] * x[2] * x[15] + x[5] * x[3] * x[14] + x[13] * x[2] * x[7] - x[13] * x[3] * x[6];
		res[6] = -x[0] * x[6] * x[15] + x[0] * x[7] * x[14] + x[4] * x[2] * x[15] - x[4] * x[3] * x[14] - x[12] * x[2] * x[7] + x[12] * x[3] * x[6];
		res[10] = x[0] * x[5] * x[15] - x[0] * x[7] * x[13] - x[4] * x[1] * x[15] + x[4] * x[3] * x[13] + x[12] * x[1] * x[7] - x[12] * x[3] * x[5];
		res[14] = -x[0] * x[5] * x[14] + x[0] * x[6] * x[13] + x[4] * x[1] * x[14] - x[4] * x[2] * x[13] - x[12] * x[1] * x[6] + x[12] * x[2] * x[5];
		res[3] = -x[1] * x[6] * x[11] + x[1] * x[7] * x[10] + x[5] * x[2] * x[11] - x[5] * x[3] * x[10] - x[9] * x[2] * x[7] + x[9] * x[3] * x[6];
		res[7] = x[0] * x[6] * x[11] - x[0] * x[7] * x[10] - x[4] * x[2] * x[11] + x[4] * x[3] * x[10] + x[8] * x[2] * x[7] - x[8] * x[3] * x[6];
		res[11] = -x[0] * x[5] * x[11] + x[0] * x[7] * x[9] + x[4] * x[1] * x[11] - x[4] * x[3] * x[9] - x[8] * x[1] * x[7] + x[8] * x[3] * x[5];
		res[15] = x[0] * x[5] * x[10] - x[0] * x[6] * x[9] - x[4] * x[1] * x[10] + x[4] * x[2] * x[9] + x[8] * x[1] * x[6] - x[8] * x[2] * x[5];
		real det = x[0] * res[0] + x[1] * res[4] + x[2] * res[8] + x[3] * res[12];
		det = 1 / det;
		res *= det;
		return res;
	}

	real determinant(const mat4 &x)
	{
		return
			x[12] * x[9] * x[6] * x[3] - x[8] * x[13] * x[6] * x[3] -
			x[12] * x[5] * x[10] * x[3] + x[4] * x[13] * x[10] * x[3] +
			x[8] * x[5] * x[14] * x[3] - x[4] * x[9] * x[14] * x[3] -
			x[12] * x[9] * x[2] * x[7] + x[8] * x[13] * x[2] * x[7] +
			x[12] * x[1] * x[10] * x[7] - x[0] * x[13] * x[10] * x[7] -
			x[8] * x[1] * x[14] * x[7] + x[0] * x[9] * x[14] * x[7] +
			x[12] * x[5] * x[2] * x[11] - x[4] * x[13] * x[2] * x[11] -
			x[12] * x[1] * x[6] * x[11] + x[0] * x[13] * x[6] * x[11] +
			x[4] * x[1] * x[14] * x[11] - x[0] * x[5] * x[14] * x[11] -
			x[8] * x[5] * x[2] * x[15] + x[4] * x[9] * x[2] * x[15] +
			x[8] * x[1] * x[6] * x[15] - x[0] * x[9] * x[6] * x[15] -
			x[4] * x[1] * x[10] * x[15] + x[0] * x[5] * x[10] * x[15];
	}

	mat4 transpose(const mat4 &x)
	{
		mat4 tmp;
		for (uint8 a = 0; a < 4; a++)
			for (uint8 b = 0; b < 4; b++)
				tmp[a * 4 + b] = x[b * 4 + a];
		return tmp;
	}

	mat4 normalize(const mat4 &x)
	{
		real len = 0;
		for (uint8 i = 0; i < 16; i++)
			len += sqr(x[i]);
		len = sqrt(len);
		return x * (1 / len);
	}

	mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up)
	{
		CAGE_ASSERT_RUNTIME(eye != target);
		CAGE_ASSERT_RUNTIME(up != vec3());
		vec3 f = normalize(target - eye);
		vec3 u = normalize(up);
		vec3 s = normalize(cross(f, u));
		u = cross(s, f);
		mat4 res;
		res[0] = s[0];
		res[4] = s[1];
		res[8] = s[2];
		res[1] = u[0];
		res[5] = u[1];
		res[9] = u[2];
		res[2] = -f[0];
		res[6] = -f[1];
		res[10] = -f[2];
		res[12] = -dot(s, eye);
		res[13] = -dot(u, eye);
		res[14] = dot(f, eye);
		return res;
	}

	mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far)
	{
		CAGE_ASSERT_RUNTIME(fov > rads(0), real(fov).value, aspectRatio.value, near.value, far.value);
		CAGE_ASSERT_RUNTIME(aspectRatio != 0, real(fov).value, aspectRatio.value, near.value, far.value);
		CAGE_ASSERT_RUNTIME(sign(near) == sign(far) && near != far, real(fov).value, aspectRatio.value, near.value, far.value);
		real f = 1 / tan(fov / 2);
		return mat4(
			f / aspectRatio, 0, 0, 0,
			0, f, 0, 0,
			0, 0, (far + near) / (near - far), -1,
			0, 0, far * near * 2.0 / (near - far), 0
		);
	}

	mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far, real zeroParallaxDistance, real eyeSeparation)
	{
		real baseLength = near * tan(fov * 0.5);
		real stereoOffset = 0.5 * eyeSeparation * near / zeroParallaxDistance;
		real left = -aspectRatio * baseLength + stereoOffset;
		real right = aspectRatio * baseLength + stereoOffset;
		real top = baseLength;
		real bottom = -baseLength;
		return perspectiveProjection(left, right, bottom, top, near, far);
	}

	mat4 perspectiveProjection(real left, real right, real bottom, real top, real near, real far)
	{
		CAGE_ASSERT_RUNTIME(left != right, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT_RUNTIME(bottom != top, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT_RUNTIME(sign(near) == sign(far) && near != far, left.value, right.value, bottom.value, top.value, near.value, far.value);
		return mat4(
			near * 2.0 / (right - left), 0, 0, 0,
			0, near * 2.0 / (top - bottom), 0, 0,
			(right + left) / (right - left), (top + bottom) / (top - bottom), -(far + near) / (far - near), -1,
			0, 0, -2 * far * near / (far - near), 0
		);
	}

	mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far)
	{
		CAGE_ASSERT_RUNTIME(left != right, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT_RUNTIME(bottom != top, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT_RUNTIME(near != far, left.value, right.value, bottom.value, top.value, near.value, far.value);
		return transpose(mat4(
			2 / (right - left), 0, 0, -(right + left) / (right - left),
			0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
			0, 0, -2 / (far - near), -(far + near) / (far - near),
			0, 0, 0, 1
		));
	}
}
