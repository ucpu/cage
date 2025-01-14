#include "math.h"

#include <cage-core/mat3x4.h>

namespace cage
{
	Mat3::Mat3(Quat other)
	{
		Real x2 = other[0] * other[0];
		Real y2 = other[1] * other[1];
		Real z2 = other[2] * other[2];
		Real xy = other[0] * other[1];
		Real xz = other[0] * other[2];
		Real yz = other[1] * other[2];
		Real wx = other[3] * other[0];
		Real wy = other[3] * other[1];
		Real wz = other[3] * other[2];
		data[0] = 1.0 - 2.0 * (y2 + z2);
		data[1] = 2.0 * (xy + wz);
		data[2] = 2.0 * (xz - wy);
		data[3] = 2.0 * (xy - wz);
		data[4] = 1.0 - 2.0 * (x2 + z2);
		data[5] = 2.0 * (yz + wx);
		data[6] = 2.0 * (xz + wy);
		data[7] = 2.0 * (yz - wx);
		data[8] = 1.0 - 2.0 * (x2 + y2);
	}

	Mat3::Mat3(Vec3 forward, Vec3 up, bool keepUp)
	{
		Vec3 right = cross(forward, up);
		if (keepUp)
			forward = cross(up, right);
		else
			up = cross(right, forward);
		forward = normalize(forward);
		up = normalize(up);
		right = cross(forward, up);
		*this = Mat3(right[0], right[1], right[2], up[0], up[1], up[2], -forward[0], -forward[1], -forward[2]);
	}

	Mat3 Mat3::parse(const String &str)
	{
		Mat3 data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 9; i++)
			data[i] = toFloat(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing mat3");
		return data;
	}

	bool Mat3::valid() const
	{
		for (Real d : data)
			if (!d.valid())
				return false;
		return true;
	}

	Mat4::Mat4(Vec3 p, Quat q, Vec3 s)
	{
		// this = T * R * S
		Mat3 r(q);
		*this = Mat4(r[0] * s[0], r[1] * s[0], r[2] * s[0], 0, r[3] * s[1], r[4] * s[1], r[5] * s[1], 0, r[6] * s[2], r[7] * s[2], r[8] * s[2], 0, p[0], p[1], p[2], 1);
	}

	Mat4 Mat4::parse(const String &str)
	{
		Mat4 data;
		String s = privat::tryRemoveParentheses(str);
		for (uint32 i = 0; i < 16; i++)
			data[i] = toFloat(privat::mathSplit(s));
		if (!s.empty())
			CAGE_THROW_ERROR(Exception, "error parsing mat4");
		return data;
	}

	bool Mat4::valid() const
	{
		for (Real d : data)
			if (!d.valid())
				return false;
		return true;
	}

	Vec3 operator*(Mat3 l, Vec3 r)
	{
		Vec3 res;
		for (uint8 i = 0; i < 3; i++)
		{
			for (uint8 j = 0; j < 3; j++)
				res[i] += r[j] * l[j * 3 + i];
		}
		return res;
	}

	Vec3 operator*(Vec3 l, Mat3 r)
	{
		return transpose(r) * l;
	}

	Mat3 operator*(Mat3 l, Mat3 r)
	{
		Mat3 res = Mat3::Zero();
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

	Mat3 transpose(Mat3 x)
	{
		Mat3 tmp;
		for (uint8 a = 0; a < 3; a++)
			for (uint8 b = 0; b < 3; b++)
				tmp[a * 3 + b] = x[b * 3 + a];
		return tmp;
	}

	Mat3 normalize(Mat3 x)
	{
		Real len;
		for (uint8 i = 0; i < 9; i++)
			len += sqr(x[i]);
		len = sqrt(len);
		return x * (1 / len);
	}

	Real determinant(Mat3 x)
	{
		return x[0 * 3 + 0] * (x[1 * 3 + 1] * x[2 * 3 + 2] - x[2 * 3 + 1] * x[1 * 3 + 2]) - x[0 * 3 + 1] * (x[1 * 3 + 0] * x[2 * 3 + 2] - x[1 * 3 + 2] * x[2 * 3 + 0]) + x[0 * 3 + 2] * (x[1 * 3 + 0] * x[2 * 3 + 1] - x[1 * 3 + 1] * x[2 * 3 + 0]);
	}

	Mat3 inverse(Mat3 x)
	{
		Real invdet = 1 / determinant(x);
		Mat3 res;
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

	Vec4 operator*(Mat4 l, Vec4 r)
	{
		Vec4 res;
		for (uint8 i = 0; i < 4; i++)
		{
			for (uint8 j = 0; j < 4; j++)
				res[i] += r[j] * l[j * 4 + i];
		}
		return res;
	}

	Vec4 operator*(Vec4 l, Mat4 r)
	{
		return transpose(r) * l;
	}

	Mat4 operator+(Mat4 l, Mat4 r)
	{
		Mat4 res;
		for (uint8 i = 0; i < 16; i++)
			res[i] = l[i] + r[i];
		return res;
	}

	Mat4 operator*(Mat4 l, Mat4 r)
	{
		Mat4 res;
		for (int col = 0; col < 4; col++)
		{
			for (int row = 0; row < 4; row++)
			{
				Real sum;
				sum += l.data[row + 0] * r.data[col * 4 + 0];
				sum += l.data[row + 4] * r.data[col * 4 + 1];
				sum += l.data[row + 8] * r.data[col * 4 + 2];
				sum += l.data[row + 12] * r.data[col * 4 + 3];
				res.data[col * 4 + row] = sum;
			}
		}
		return res;
	}

	Mat4 inverse(Mat4 x)
	{
		Mat4 res;
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
		Real det = x[0] * res[0] + x[1] * res[4] + x[2] * res[8] + x[3] * res[12];
		det = 1 / det;
		res *= det;
		return res;
	}

	Real determinant(Mat4 x)
	{
		return x[12] * x[9] * x[6] * x[3] - x[8] * x[13] * x[6] * x[3] - x[12] * x[5] * x[10] * x[3] + x[4] * x[13] * x[10] * x[3] + x[8] * x[5] * x[14] * x[3] - x[4] * x[9] * x[14] * x[3] - x[12] * x[9] * x[2] * x[7] + x[8] * x[13] * x[2] * x[7] + x[12] * x[1] * x[10] * x[7] - x[0] * x[13] * x[10] * x[7] - x[8] * x[1] * x[14] * x[7] + x[0] * x[9] * x[14] * x[7] + x[12] * x[5] * x[2] * x[11] - x[4] * x[13] * x[2] * x[11] - x[12] * x[1] * x[6] * x[11] + x[0] * x[13] * x[6] * x[11] + x[4] * x[1] * x[14] * x[11] - x[0] * x[5] * x[14] * x[11] - x[8] * x[5] * x[2] * x[15] + x[4] * x[9] * x[2] * x[15] + x[8] * x[1] * x[6] * x[15] - x[0] * x[9] * x[6] * x[15] - x[4] * x[1] * x[10] * x[15] + x[0] * x[5] * x[10] * x[15];
	}

	Mat4 transpose(Mat4 x)
	{
		Mat4 tmp;
		for (uint8 a = 0; a < 4; a++)
			for (uint8 b = 0; b < 4; b++)
				tmp[a * 4 + b] = x[b * 4 + a];
		return tmp;
	}

	Mat4 normalize(Mat4 x)
	{
		Real len = 0;
		for (uint8 i = 0; i < 16; i++)
			len += sqr(x[i]);
		len = sqrt(len);
		return x * (1 / len);
	}

	Mat3x4::Mat3x4() : Mat3x4(Mat4()) {}

	Mat3x4::Mat3x4(Mat3 in)
	{
		for (uint32 a = 0; a < 3; a++)
			for (uint32 b = 0; b < 3; b++)
				data[b][a] = in[a * 3 + b];
	}

	Mat3x4::Mat3x4(Mat4 in)
	{
		CAGE_ASSERT(abs(in[3]) < 1e-5 && abs(in[7]) < 1e-5 && abs(in[11]) < 1e-5 && abs(in[15] - 1) < 1e-5);
		for (uint32 a = 0; a < 4; a++)
			for (uint32 b = 0; b < 3; b++)
				data[b][a] = in[a * 4 + b];
	}

	Mat3x4::operator Mat4() const
	{
		Mat4 r;
		for (uint32 a = 0; a < 4; a++)
			for (uint32 b = 0; b < 3; b++)
				r[a * 4 + b] = data[b][a];
		return r;
	}
}
