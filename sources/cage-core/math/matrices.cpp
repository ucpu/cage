#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>

namespace cage
{
	mat3::mat3()
	{
		data[0] = 1;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;
		data[4] = 1;
		data[5] = 0;
		data[6] = 0;
		data[7] = 0;
		data[8] = 1;
	}

	mat3::mat3(const real other[9])
	{
		for (uint8 i = 0; i < 9; i++)
			data[i] = other[i];
	}

	mat3::mat3(real a, real b, real c, real d, real e, real f, real g, real h, real i)
	{
		data[0] = a;
		data[1] = b;
		data[2] = c;
		data[3] = d;
		data[4] = e;
		data[5] = f;
		data[6] = g;
		data[7] = h;
		data[8] = i;
	}

	mat3::mat3(const quat &other)
	{
		real x2 = other.data[0] * other.data[0];
		real y2 = other.data[1] * other.data[1];
		real z2 = other.data[2] * other.data[2];
		real xy = other.data[0] * other.data[1];
		real xz = other.data[0] * other.data[2];
		real yz = other.data[1] * other.data[2];
		real wx = other.data[3] * other.data[0];
		real wy = other.data[3] * other.data[1];
		real wz = other.data[3] * other.data[2];
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
		forward = forward.normalize();
		up = up.normalize();
		right = forward.cross(up);
		*this = mat3(
			right[0], up[0], -forward[0],
			right[1], up[1], -forward[1],
			right[2], up[2], -forward[2]
		).transpose();
	}

	mat3::mat3(const mat4 &other)
	{
		data[0] = other.data[0];
		data[1] = other.data[1];
		data[2] = other.data[2];
		data[3] = other.data[4];
		data[4] = other.data[5];
		data[5] = other.data[6];
		data[6] = other.data[8];
		data[7] = other.data[9];
		data[8] = other.data[10];
	}

	mat3 mat3::operator * (real other) const
	{
		mat3 tmp = *this;
		for (uint8 i = 0; i < 9; i++)
			tmp.data[i] *= other;
		return tmp;
	}

	vec3 mat3::operator * (const vec3 &other) const
	{
		vec3 res;
		for (uint8 i = 0; i < 3; i++)
		{
			for (uint8 j = 0; j < 3; j++)
				res.data[i] += other.data[j] * data[j * 3 + i];
		}
		return res;
	}

	mat3 mat3::operator + (const mat3 &other) const
	{
		mat3 res;
		for (uint8 i = 0; i < 9; i++)
			res.data[i] = data[i] + other.data[i];
		return res;
	}

	mat3 mat3::operator * (const mat3 &other) const
	{
		mat3 res;
		for (uint8 x = 0; x < 3; x++)
		{
			for (uint8 y = 0; y < 3; y++)
			{
				res.data[y * 3 + x] = 0;
				for (uint8 z = 0; z < 3; z++)
					res.data[y * 3 + x] += data[z * 3 + x] * other.data[y * 3 + z];
			}
		}
		return res;
	}

	mat3 mat3::transpose() const
	{
		mat3 tmp = *this;
		for (uint8 a = 0; a < 3; a++)
			for (uint8 b = 0; b < 3; b++)
				tmp.data[a * 3 + b] = data[b * 3 + a];
		return tmp;
	}

	mat3 mat3::normalize() const
	{
		real len = 0;
		for (uint8 i = 0; i < 9; i++)
			len += data[i].sqr();
		len = len.sqrt();
		return *this * (1 / len);
	}

	real mat3::determinant() const
	{
		return
			data[0 * 3 + 0] * (data[1 * 3 + 1] * data[2 * 3 + 2] - data[2 * 3 + 1] * data[1 * 3 + 2])
			- data[0 * 3 + 1] * (data[1 * 3 + 0] * data[2 * 3 + 2] - data[1 * 3 + 2] * data[2 * 3 + 0])
			+ data[0 * 3 + 2] * (data[1 * 3 + 0] * data[2 * 3 + 1] - data[1 * 3 + 1] * data[2 * 3 + 0]);
	}

	bool mat3::valid() const
	{
		for (uint8 i = 0; i < 9; i++)
			if (!data[i].valid())
				return false;
		return true;
	}

	mat3 mat3::inverse() const
	{
		real invdet = (real)1 / determinant();
		mat3 res;
		res.data[0 * 3 + 0] = (data[1 * 3 + 1] * data[2 * 3 + 2] - data[2 * 3 + 1] * data[1 * 3 + 2]) * invdet;
		res.data[1 * 3 + 0] = -(data[0 * 3 + 1] * data[2 * 3 + 2] - data[0 * 3 + 2] * data[2 * 3 + 1]) * invdet;
		res.data[2 * 3 + 0] = (data[0 * 3 + 1] * data[1 * 3 + 2] - data[0 * 3 + 2] * data[1 * 3 + 1]) * invdet;
		res.data[0 * 3 + 1] = -(data[1 * 3 + 0] * data[2 * 3 + 2] - data[1 * 3 + 2] * data[2 * 3 + 0]) * invdet;
		res.data[1 * 3 + 1] = (data[0 * 3 + 0] * data[2 * 3 + 2] - data[0 * 3 + 2] * data[2 * 3 + 0]) * invdet;
		res.data[2 * 3 + 1] = -(data[0 * 3 + 0] * data[1 * 3 + 2] - data[1 * 3 + 0] * data[0 * 3 + 2]) * invdet;
		res.data[0 * 3 + 2] = (data[1 * 3 + 0] * data[2 * 3 + 1] - data[2 * 3 + 0] * data[1 * 3 + 1]) * invdet;
		res.data[1 * 3 + 2] = -(data[0 * 3 + 0] * data[2 * 3 + 1] - data[2 * 3 + 0] * data[0 * 3 + 1]) * invdet;
		res.data[2 * 3 + 2] = (data[0 * 3 + 0] * data[1 * 3 + 1] - data[1 * 3 + 0] * data[0 * 3 + 1]) * invdet;
		return res.transpose();
	}

	mat4::mat4()
	{
		data[0] = 1;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;
		data[4] = 0;
		data[5] = 1;
		data[6] = 0;
		data[7] = 0;
		data[8] = 0;
		data[9] = 0;
		data[10] = 1;
		data[11] = 0;
		data[12] = 0;
		data[13] = 0;
		data[14] = 0;
		data[15] = 1;
	}

	mat4::mat4(const real other[16])
	{
		for (uint8 i = 0; i < 16; i++)
			data[i] = other[i];
	}

	mat4::mat4(real a, real b, real c, real d, real e, real f, real g, real h, real i, real j, real k, real l, real m, real n, real o, real p)
	{
		data[0] = a;
		data[1] = b;
		data[2] = c;
		data[3] = d;
		data[4] = e;
		data[5] = f;
		data[6] = g;
		data[7] = h;
		data[8] = i;
		data[9] = j;
		data[10] = k;
		data[11] = l;
		data[12] = m;
		data[13] = n;
		data[14] = o;
		data[15] = p;
	}

	mat4::mat4(real other)
	{
		data[0] = other;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;
		data[4] = 0;
		data[5] = other;
		data[6] = 0;
		data[7] = 0;
		data[8] = 0;
		data[9] = 0;
		data[10] = other;
		data[11] = 0;
		data[12] = 0;
		data[13] = 0;
		data[14] = 0;
		data[15] = 1;
	}

	mat4::mat4(const mat3 &other)
	{
		data[0] = other.data[0];
		data[1] = other.data[1];
		data[2] = other.data[2];
		data[3] = 0;
		data[4] = other.data[3];
		data[5] = other.data[4];
		data[6] = other.data[5];
		data[7] = 0;
		data[8] = other.data[6];
		data[9] = other.data[7];
		data[10] = other.data[8];
		data[11] = 0;
		data[12] = 0;
		data[13] = 0;
		data[14] = 0;
		data[15] = 1;
	}

	mat4::mat4(const vec3 &other)
	{
		data[0] = 1;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;
		data[4] = 0;
		data[5] = 1;
		data[6] = 0;
		data[7] = 0;
		data[8] = 0;
		data[9] = 0;
		data[10] = 1;
		data[11] = 0;
		data[12] = other.data[0];
		data[13] = other.data[1];
		data[14] = other.data[2];
		data[15] = 1;
	}

	mat4::mat4(const vec3 &position, const quat &orientation, const vec3 &scale)
	{
		*this = mat4(orientation);
		for (uint32 i = 0; i < 3; i++)
		{
			data[0 + i * 4] *= scale[i];
			data[1 + i * 4] *= scale[i];
			data[2 + i * 4] *= scale[i];
		}
		data[12] = position[0];
		data[13] = position[1];
		data[14] = position[2];
	}

	mat4 mat4::operator * (real other) const
	{
		mat4 tmp = *this;
		for (uint8 i = 0; i < 16; i++)
			tmp.data[i] *= other;
		return tmp;
	}

	vec4 mat4::operator * (const vec4 &other) const
	{
		vec4 res;
		for (uint8 i = 0; i < 4; i++)
		{
			for (uint8 j = 0; j < 4; j++)
				res.data[i] += other.data[j] * data[j * 4 + i];
		}
		return res;
	}

	mat4 mat4::operator + (const mat4 &other) const
	{
		mat4 res;
		for (uint8 i = 0; i < 16; i++)
			res.data[i] = data[i] + other.data[i];
		return res;
	}

	mat4 mat4::operator * (const mat4 &other) const
	{
		mat4 res;
		for (uint8 x = 0; x < 4; x++)
		{
			for (uint8 y = 0; y < 4; y++)
			{
				res.data[y * 4 + x] = 0;
				for (uint8 z = 0; z < 4; z++)
					res.data[y * 4 + x] += data[z * 4 + x] * other.data[y * 4 + z];
			}
		}
		return res;
	}

	mat4 mat4::inverse() const
	{
		mat4 res;

		res.data[0] = data[5] * data[10] * data[15] -
			data[5] * data[11] * data[14] -
			data[9] * data[6] * data[15] +
			data[9] * data[7] * data[14] +
			data[13] * data[6] * data[11] -
			data[13] * data[7] * data[10];

		res.data[4] = -data[4] * data[10] * data[15] +
			data[4] * data[11] * data[14] +
			data[8] * data[6] * data[15] -
			data[8] * data[7] * data[14] -
			data[12] * data[6] * data[11] +
			data[12] * data[7] * data[10];

		res.data[8] = data[4] * data[9] * data[15] -
			data[4] * data[11] * data[13] -
			data[8] * data[5] * data[15] +
			data[8] * data[7] * data[13] +
			data[12] * data[5] * data[11] -
			data[12] * data[7] * data[9];

		res.data[12] = -data[4] * data[9] * data[14] +
			data[4] * data[10] * data[13] +
			data[8] * data[5] * data[14] -
			data[8] * data[6] * data[13] -
			data[12] * data[5] * data[10] +
			data[12] * data[6] * data[9];

		res.data[1] = -data[1] * data[10] * data[15] +
			data[1] * data[11] * data[14] +
			data[9] * data[2] * data[15] -
			data[9] * data[3] * data[14] -
			data[13] * data[2] * data[11] +
			data[13] * data[3] * data[10];

		res.data[5] = data[0] * data[10] * data[15] -
			data[0] * data[11] * data[14] -
			data[8] * data[2] * data[15] +
			data[8] * data[3] * data[14] +
			data[12] * data[2] * data[11] -
			data[12] * data[3] * data[10];

		res.data[9] = -data[0] * data[9] * data[15] +
			data[0] * data[11] * data[13] +
			data[8] * data[1] * data[15] -
			data[8] * data[3] * data[13] -
			data[12] * data[1] * data[11] +
			data[12] * data[3] * data[9];

		res.data[13] = data[0] * data[9] * data[14] -
			data[0] * data[10] * data[13] -
			data[8] * data[1] * data[14] +
			data[8] * data[2] * data[13] +
			data[12] * data[1] * data[10] -
			data[12] * data[2] * data[9];

		res.data[2] = data[1] * data[6] * data[15] -
			data[1] * data[7] * data[14] -
			data[5] * data[2] * data[15] +
			data[5] * data[3] * data[14] +
			data[13] * data[2] * data[7] -
			data[13] * data[3] * data[6];

		res.data[6] = -data[0] * data[6] * data[15] +
			data[0] * data[7] * data[14] +
			data[4] * data[2] * data[15] -
			data[4] * data[3] * data[14] -
			data[12] * data[2] * data[7] +
			data[12] * data[3] * data[6];

		res.data[10] = data[0] * data[5] * data[15] -
			data[0] * data[7] * data[13] -
			data[4] * data[1] * data[15] +
			data[4] * data[3] * data[13] +
			data[12] * data[1] * data[7] -
			data[12] * data[3] * data[5];

		res.data[14] = -data[0] * data[5] * data[14] +
			data[0] * data[6] * data[13] +
			data[4] * data[1] * data[14] -
			data[4] * data[2] * data[13] -
			data[12] * data[1] * data[6] +
			data[12] * data[2] * data[5];

		res.data[3] = -data[1] * data[6] * data[11] +
			data[1] * data[7] * data[10] +
			data[5] * data[2] * data[11] -
			data[5] * data[3] * data[10] -
			data[9] * data[2] * data[7] +
			data[9] * data[3] * data[6];

		res.data[7] = data[0] * data[6] * data[11] -
			data[0] * data[7] * data[10] -
			data[4] * data[2] * data[11] +
			data[4] * data[3] * data[10] +
			data[8] * data[2] * data[7] -
			data[8] * data[3] * data[6];

		res.data[11] = -data[0] * data[5] * data[11] +
			data[0] * data[7] * data[9] +
			data[4] * data[1] * data[11] -
			data[4] * data[3] * data[9] -
			data[8] * data[1] * data[7] +
			data[8] * data[3] * data[5];

		res.data[15] = data[0] * data[5] * data[10] -
			data[0] * data[6] * data[9] -
			data[4] * data[1] * data[10] +
			data[4] * data[2] * data[9] +
			data[8] * data[1] * data[6] -
			data[8] * data[2] * data[5];

		real det = data[0] * res.data[0] + data[1] * res.data[4] + data[2] * res.data[8] + data[3] * res.data[12];

		//if (det == 0)
		//	CAGE_THROW_ERROR(exception, "singular matrix");

		det = (real)1.0 / det;

		for (uint8 i = 0; i < 16; i++)
			res.data[i] *= det;

		return res;
	}

	real mat4::determinant() const
	{
		return
			data[12] * data[9] * data[6] * data[3] - data[8] * data[13] * data[6] * data[3] -
			data[12] * data[5] * data[10] * data[3] + data[4] * data[13] * data[10] * data[3] +
			data[8] * data[5] * data[14] * data[3] - data[4] * data[9] * data[14] * data[3] -
			data[12] * data[9] * data[2] * data[7] + data[8] * data[13] * data[2] * data[7] +
			data[12] * data[1] * data[10] * data[7] - data[0] * data[13] * data[10] * data[7] -
			data[8] * data[1] * data[14] * data[7] + data[0] * data[9] * data[14] * data[7] +
			data[12] * data[5] * data[2] * data[11] - data[4] * data[13] * data[2] * data[11] -
			data[12] * data[1] * data[6] * data[11] + data[0] * data[13] * data[6] * data[11] +
			data[4] * data[1] * data[14] * data[11] - data[0] * data[5] * data[14] * data[11] -
			data[8] * data[5] * data[2] * data[15] + data[4] * data[9] * data[2] * data[15] +
			data[8] * data[1] * data[6] * data[15] - data[0] * data[9] * data[6] * data[15] -
			data[4] * data[1] * data[10] * data[15] + data[0] * data[5] * data[10] * data[15];
	}

	mat4 mat4::transpose() const
	{
		mat4 tmp;
		for (uint8 a = 0; a < 4; a++)
			for (uint8 b = 0; b < 4; b++)
				tmp.data[a * 4 + b] = data[b * 4 + a];
		return tmp;
	}

	mat4 mat4::normalize() const
	{
		real len = 0;
		for (uint8 i = 0; i < 16; i++)
			len += data[i].sqr();
		len = len.sqrt();
		return *this * (1 / len);
	}

	bool mat4::valid() const
	{
		for (uint8 i = 0; i < 16; i++)
			if (!data[i].valid())
				return false;
		return true;
	}

	mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up)
	{
		CAGE_ASSERT_RUNTIME(eye != target);
		CAGE_ASSERT_RUNTIME(up != vec3());
		vec3 f = (target - eye).normalize();
		vec3 u = up.normalize();
		vec3 s = f.cross(u).normalize();
		u = s.cross(f);
		mat4 res;
		res[0] = s.data[0];
		res[4] = s.data[1];
		res[8] = s.data[2];
		res[1] = u.data[0];
		res[5] = u.data[1];
		res[9] = u.data[2];
		res[2] = -f.data[0];
		res[6] = -f.data[1];
		res[10] = -f.data[2];
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
		return mat4(
			2 / (right - left), 0, 0, -(right + left) / (right - left),
			0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
			0, 0, -2 / (far - near), -(far + near) / (far - near),
			0, 0, 0, 1
		).transpose();
	}

	mat3 modelToNormal(const mat4 &value)
	{
		return mat3(value[0], value[1], value[2],
			value[4], value[5], value[6],
			value[8], value[9], value[10]).inverse().transpose();
	}
}