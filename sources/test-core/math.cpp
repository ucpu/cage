#include "main.h"
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/timer.h>
#include <cage-core/macros.h>

#include <cmath>

void test(Real a, Real b)
{
	Real d = abs(a - b);
	CAGE_TEST(d < 1e-4);
}
void test(sint32 a, sint32 b)
{
	CAGE_TEST(a == b);
}
void test(const Mat4 &a, const Mat4 &b)
{
	for (uint32 i = 0; i < 16; i++)
		test(a[i], b[i]);
}
void test(const Mat3 &a, const Mat3 &b)
{
	for (uint32 i = 0; i < 9; i++)
		test(a[i], b[i]);
}
void test(const Vec4 &a, const Vec4 &b)
{
	for (uint32 i = 0; i < 4; i++)
		test(a[i], b[i]);
}
void test(const Vec3 &a, const Vec3 &b)
{
	for (uint32 i = 0; i < 3; i++)
		test(a[i], b[i]);
}
void test(const Vec2 &a, const Vec2 &b)
{
	for (uint32 i = 0; i < 2; i++)
		test(a[i], b[i]);
}
void test(const Vec4i &a, const Vec4i &b)
{
	for (uint32 i = 0; i < 4; i++)
		test(a[i], b[i]);
}
void test(const Vec3i &a, const Vec3i &b)
{
	for (uint32 i = 0; i < 3; i++)
		test(a[i], b[i]);
}
void test(const Vec2i &a, const Vec2i &b)
{
	for (uint32 i = 0; i < 2; i++)
		test(a[i], b[i]);
}
void test(const Quat &a, const Quat &b)
{
	test(abs(dot(a, b)), 1);
	test(a * Vec3(0, 0, 0.1), b * Vec3(0, 0, 0.1));
	test(a * Vec3(0, 0.1, 0), b * Vec3(0, 0.1, 0));
	test(a * Vec3(0.1, 0, 0), b * Vec3(0.1, 0, 0));
}
void test(Rads a, Rads b)
{
	if (a < Rads(0))
		a += Rads::Full();
	if (b < Rads(0))
		b += Rads::Full();
	test(Real(a), Real(b));
}

namespace
{
	template<class T> struct MakeNotReal {};
	template<> struct MakeNotReal<Vec2> { using type = Vec2i; };
	template<> struct MakeNotReal<Vec3> { using type = Vec3i; };
	template<> struct MakeNotReal<Vec4> { using type = Vec4i; };
	template<> struct MakeNotReal<Vec2i> { using type = Vec2; };
	template<> struct MakeNotReal<Vec3i> { using type = Vec3; };
	template<> struct MakeNotReal<Vec4i> { using type = Vec4; };

	template<class T, uint32 N>
	void checkVectorsCommon()
	{
		{
			T r;
			T a;
			constexpr T b = T(42);
			r = a;
			r = b;
#define GCHL_GENERATE(OPERATOR) \
			r = b OPERATOR b; \
			r = b OPERATOR 5; \
			r = 5 OPERATOR b;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, / ));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
			r = a OPERATOR b; \
			r = a OPERATOR 5;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +=, -=, *=, /=));
#undef GCHL_GENERATE
			-b;
			+b;
			for (uint32 i = 0; i < N; i++)
				r[i] = b[i];
			CAGE_TEST(b == b);
			CAGE_TEST(!(b != b));
		}

		{ // stringizer as r-Value
			T a;
			String s = Stringizer() + a;
			T b = T::parse(s);
			CAGE_TEST(a == b);
		}

		{ // stringizer as l-Value
			T a;
			Stringizer s;
			s + a;
			T b = T::parse(s);
			CAGE_TEST(a == b);
		}

		{
			constexpr T a = T(0);
			constexpr T b = T(0);

			constexpr T k1 = min(a, b);
			constexpr T k2 = max(a, b);
			constexpr T k3 = min(a, 5);
			constexpr T k4 = max(a, 5);
			constexpr T k5 = min(5, a);
			constexpr T k6 = max(5, a);
			constexpr T k7 = clamp(a, b, b);
			constexpr T k8 = clamp(a, 1, 9);
			constexpr T k9 = abs(a);

			constexpr T other = T(typename MakeNotReal<T>::type(5));
			test(other[0], 5);
			test(other[N-1], 5);
		}

		T t(42);
	}

	template<class T, uint32 N>
	void checkVectorsReal()
	{
		{
			T r, a, b;
			r = a;
			r = b;
#define GCHL_GENERATE(OPERATOR) \
			r = b OPERATOR 5.5f; \
			r = 5.5f OPERATOR b; \
			r = b OPERATOR 5.5; \
			r = 5.5 OPERATOR b;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, /));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
			r = a OPERATOR 5.5f; \
			r = a OPERATOR 5.f;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +=, -=, *=, /=));
#undef GCHL_GENERATE
		}

		{
			constexpr T a = T(0);
			constexpr T b = T(0);
			a.valid();

			normalize(a);
			constexpr T k1 = min(a, 0.5);
			constexpr T k2 = max(a, 0.5);
			constexpr T k3 = min(0.5, a);
			constexpr T k4 = max(0.5, a);
			constexpr T k5 = clamp(a, 0.1, 0.9);
			constexpr T k6 = saturate(a);
			length(a);
			constexpr Real k7 = lengthSquared(a);
			constexpr Real k8 = dot(a, b);
			valid(a);
			constexpr T k9 = interpolate(a, b, 0.5);
			distance(a, b);
			constexpr Real k10 = distanceSquared(a, b);
		}

		T t(3.5);
	}

	void testMathCompiles()
	{
		CAGE_TESTCASE("compile tests");
		checkVectorsCommon<Vec2, 2>();
		checkVectorsCommon<Vec3, 3>();
		checkVectorsCommon<Vec4, 4>();
		checkVectorsCommon<Vec2i, 2>();
		checkVectorsCommon<Vec3i, 3>();
		checkVectorsCommon<Vec4i, 4>();
		checkVectorsReal<Vec2, 2>();
		checkVectorsReal<Vec3, 3>();
		checkVectorsReal<Vec4, 4>();

		{
			constexpr Vec2 a = Vec2(Vec3(42));
			constexpr Vec2 b = Vec2(Vec4(42));
			constexpr Vec3 c = Vec3(Vec4(42));
			constexpr Vec2i d = Vec2i(Vec3i(42));
			constexpr Vec2i e = Vec2i(Vec4i(42));
			constexpr Vec3i f = Vec3i(Vec4i(42));
		}
	}

	void testMathReal()
	{
		CAGE_TESTCASE("real");

		{
			CAGE_TESTCASE("basics");
			CAGE_TEST(Real::Pi() > 3.1 && Real::Pi() < 3.2);
			CAGE_TEST(Real::E() > 2.7 && Real::E() < 2.8);
			CAGE_TEST(Real(Real::Infinity()).valid());
			CAGE_TEST(!Real(Real::Nan()).valid());
			CAGE_TEST(!Real(Real::Infinity()).finite());
			CAGE_TEST(!Real(Real::Nan()).finite());
			CAGE_TEST(Real(Real::Pi()).finite());
			CAGE_TEST(Real(0).finite());
			CAGE_TEST(Real(42).finite());
			CAGE_TEST(Real(42).valid());
			constexpr Real a = 3;
			test(a + 2, 5);
			test(2 + a, 5);
			test(a - 2, 1);
			test(2 - a, -1);
			test(a * 2, 6);
			test(2 * a, 6);
			test(a / 2, 1.5);
			test(2 / a, 2.f / 3.f);
			CAGE_TEST(valid(3.0));
			CAGE_TEST(valid(3.0f));
		}

		{
			CAGE_TESTCASE("modulo");
			test(Real(+15) % Real(+12), +15 % +12); // +3
			test(Real(+15) % Real(-12), +15 % -12); // +3
			test(Real(-15) % Real(-12), -15 % -12); // -3
			test(Real(-15) % Real(+12), -15 % +12); // -3
			test(Real(+150) % Real(+13), +150 % +13); // 7
			test(Real(+150) % Real(-13), +150 % -13); // 7
			test(Real(-150) % Real(-13), -150 % -13); // -7
			test(Real(-150) % Real(+13), -150 % +13); // -7
			test(Real(+5) % Real(+1.8), +1.4);
			test(Real(+5) % Real(-1.8), +1.4);
			test(Real(-5) % Real(-1.8), -1.4);
			test(Real(-5) % Real(+1.8), -1.4);
		}

		{
			CAGE_TESTCASE("wrap");
			test(wrap(0.9 * 15), Real(0.9 * 15) % 1);
			test(wrap(0), 0);
			test(wrap(-0.75), 0.25);
			test(wrap(-0.5), 0.5);
			test(wrap(-0.25), 0.75);
			test(wrap(0.25), 0.25);
			test(wrap(0.5), 0.5);
			test(wrap(0.75), 0.75);
		}

		{
			CAGE_TESTCASE("distanceWrap");
			test(distanceWrap(0.25, 0.5), 0.25);
			test(distanceWrap(0.75, 0.5), 0.25);
			test(distanceWrap(0.5, 0.25), 0.25);
			test(distanceWrap(0.5, 0.75), 0.25);
			test(distanceWrap(0.5, 0.5), 0.0);
			test(distanceWrap(0.1, 0.9), 0.2);
			test(distanceWrap(0.1, 0.7), 0.4);
			test(distanceWrap(0.1, 0.5), 0.4);
			test(distanceWrap(0.1, 0.3), 0.2);
			test(distanceWrap(-0.1, -0.3), 0.2);
			test(distanceWrap(-0.1, 0.3), 0.4);
			test(distanceWrap(0.1, -0.3), 0.4);
			test(distanceWrap(0.1, -0.7), 0.2);
			test(distanceWrap(-0.1, 0.7), 0.2);
			test(distanceWrap(15.5, 0.7), 0.2);
		}

		{
			CAGE_TESTCASE("interpolate");
			test(interpolate(Real(5), Real(15), 0.5), 10);
			test(interpolate(Real(5), Real(15), 0), 5);
			test(interpolate(Real(5), Real(15), 1), 15);
			test(interpolate(Real(5), Real(15), 0.2), 7);
		}

		{
			CAGE_TESTCASE("interpolateWrap");
			test(interpolateWrap(0.1, 0.5, 0), 0.1);
			test(interpolateWrap(0.1, 0.5, 0.5), 0.3);
			test(interpolateWrap(0.1, 0.5, 1), 0.5);
			test(interpolateWrap(1.1, 1.5, 0.5), 0.3);
			test(interpolateWrap(0.1, 13.5, 0.5), 0.3);
			test(interpolateWrap(-0.1, -0.5, 0), 0.9);
			test(interpolateWrap(-0.1, -0.5, 0.5), 0.7);
			test(interpolateWrap(-0.1, -0.5, 1), 0.5);
			test(interpolateWrap(-0.1, 0.5, 0.5), 0.7);
			test(interpolateWrap(0.1, -0.5, 0.5), 0.3);
			test(interpolateWrap(0.1, 0.9, 0), 0.1);
			test(interpolateWrap(0.1, 0.9, 0.25), 0.05);
			test(interpolateWrap(0.1, 0.9, 0.75), 0.95);
			test(interpolateWrap(0.1, 0.9, 1), 0.9);
			test(interpolateWrap(0.1, 0.7, 0), 0.1);
			test(interpolateWrap(0.1, 0.7, 0.5), 0.9);
			test(interpolateWrap(0.1, 0.7, 1), 0.7);
			test(interpolateWrap(0.9, 0.3, 0), 0.9);
			test(interpolateWrap(0.9, 0.3, 0.5), 0.1);
			test(interpolateWrap(0.9, 0.3, 1), 0.3);
		}

		{
			CAGE_TESTCASE("clamp");
			CAGE_TEST(clamp(5, 3, 7) == 5);
			CAGE_TEST(clamp(1, 3, 7) == 3);
			CAGE_TEST(clamp(9, 3, 7) == 7);
			CAGE_TEST(clamp(5.f, 3.f, 7.f) == 5.f);
			CAGE_TEST(clamp(1.f, 3.f, 7.f) == 3.f);
			CAGE_TEST(clamp(9.f, 3.f, 7.f) == 7.f);
		}

		{
			CAGE_TESTCASE("saturate");
			test(saturate(4.5), (Real)1.0);
			test(saturate(1.5), (Real)1.0);
			test(saturate(0.65), (Real)0.65);
			test(saturate(0.25), (Real)0.25);
			test(saturate(-0.55), (Real)0.0);
			test(saturate(-3.55), (Real)0.0);
		}

		{
			CAGE_TESTCASE("rounding");
			CAGE_TEST(round(Real(5.2)) == 5);
			CAGE_TEST(round(Real(4.8)) == 5);
			CAGE_TEST(floor(Real(5.2)) == 5);
			CAGE_TEST(floor(Real(4.8)) == 4);
			CAGE_TEST(ceil(Real(5.2)) == 6);
			CAGE_TEST(ceil(Real(4.8)) == 5);
			CAGE_TEST(round(Real(-5.2)) == -5);
			CAGE_TEST(round(Real(-4.8)) == -5);
			CAGE_TEST(floor(Real(-5.2)) == -6);
			CAGE_TEST(floor(Real(-4.8)) == -5);
			CAGE_TEST(ceil(Real(-5.2)) == -5);
			CAGE_TEST(ceil(Real(-4.8)) == -4);
		}
	}

	void testMathAngles()
	{
		CAGE_TESTCASE("degs & rads");
		test(Rads(Degs(0)), Rads(0));
		test(Rads(Degs(90)), Rads(Real::Pi() / 2));
		test(Rads(Degs(180)), Rads(Real::Pi()));
		test(Rads(wrap(Degs(90 * 15))), Degs((90 * 15) % 360));
		test(Rads(wrap(Degs(0))), Degs(0));
		test(Rads(wrap(Degs(-270))), Degs(90));
		test(Rads(wrap(Degs(-180))), Degs(180));
		test(Rads(wrap(Degs(-90))), Degs(270));
		test(Rads(wrap(Degs(360))), Degs(0));
		test(Rads(wrap(Degs(90))), Degs(90));
		test(Rads(wrap(Degs(180))), Degs(180));
		test(Rads(wrap(Degs(270))), Degs(270));

		{
			CAGE_TESTCASE("operators");
			Rads angle;
			Real scalar;
			angle = angle + angle;
			angle = angle - angle;
			angle = angle * scalar;
			angle = scalar * angle;
			angle = angle / scalar;
			angle = angle / angle;
		}

		{
			CAGE_TESTCASE("atan2");
			for (uint32 i = 0; i < 10; i++)
			{
				Rads a = wrap(randomAngle());
				Real d = randomRange(0.1, 10.0);
				Real x = cos(a) * d;
				Real y = sin(a) * d;
				Rads r = wrap(atan2(x, y));
				test(a, r);
			}

			for (uint32 i = 0; i < 10; i++)
			{
				Rads a = randomAngle();
				Real d = randomRange(0.1, 10.0);
				Real x = cos(a) * d;
				Real y = sin(a) * d;
				Rads c = atan2(x, y);
				Rads s = Rads(std::atan2(y.value, x.value));
				test(c, s);
			}
		}
	}

	void testMathVec2()
	{
		CAGE_TESTCASE("vec2");
		constexpr Vec2 a(3, 5);
		test(a[0], 3);
		test(a[1], 5);
		constexpr Vec2 b(2, 1);
		constexpr Vec2 c(5, 2);
		test(a, Vec2(3, 5));
		CAGE_TEST(a != b);
		test(a + b, Vec2(5, 6));
		test(a * b, Vec2(6, 5));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, c), Vec2(5, 5));
		test(min(a, c), Vec2(3, 2));
		constexpr Vec2 d = a + c;
		test(d, Vec2(8, 7));

		test(Vec2(1, 5), Vec2(1, 5));
		CAGE_TEST(!(Vec2(1, 5) != Vec2(1, 5)));
		CAGE_TEST(Vec2(3, 5) != Vec2(1, 5));
	}

	void testMathVec3()
	{
		CAGE_TESTCASE("vec3");
		constexpr Vec3 a(3, 5, 1);
		constexpr Vec3 b(1, 1, 4);
		test(a[0], 3);
		test(a[1], 5);
		test(a[2], 1);
		test(a, Vec3(3, 5, 1));
		test(Vec3(1, 1, 4), b);
		CAGE_TEST(a != b);
		test(a, a);
		test(a + b, Vec3(4, 6, 5));
		test(a * b, Vec3(3, 5, 4));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, b), Vec3(3, 5, 4));
		test(min(a, b), Vec3(1, 1, 1));
		constexpr Vec3 c = a + b;
		test(c, Vec3(4, 6, 5));
		test(cross(a, b), Vec3(19, -11, -2));
		test(dot(a, b), 12);
		test(a / b, Vec3(3, 5, 1.f / 4));
		test(8 / b, Vec3(8, 8, 2));
		test(a / (1.f / 3), Vec3(9, 15, 3));

		test(Vec3(1, 5, 4), Vec3(1, 5, 4));
		CAGE_TEST(!(Vec3(1, 5, 4) != Vec3(1, 5, 4)));
		CAGE_TEST(Vec3(3, 5, 4) != Vec3(1, 5, 4));

		CAGE_TEST(clamp(Vec3(1, 3, 5), Vec3(2), Vec3(4)) == Vec3(2, 3, 4));
		CAGE_TEST(clamp(Vec3(1, 3, 5), 2, 4) == Vec3(2, 3, 4));

		CAGE_TEST(abs(Vec3(1, 3, 5)) == Vec3(1, 3, 5));
		CAGE_TEST(abs(Vec3(1, -3, 5)) == Vec3(1, 3, 5));
		CAGE_TEST(abs(Vec3(-1, 3, -5)) == Vec3(1, 3, 5));
		CAGE_TEST(abs(Vec3(-1, -3, -5)) == Vec3(1, 3, 5));

		test(dominantAxis(Vec3(13, -4, 1)), Vec3(1, 0, 0));
		test(dominantAxis(Vec3(-3, -15, 4)), Vec3(0, -1, 0));

		// right = forward x up (in this order)
		test(cross(Vec3(0, 0, -1), Vec3(0, 1, 0)), Vec3(1, 0, 0));
	}

	void testMathVec4()
	{
		CAGE_TESTCASE("vec4");
		constexpr Vec4 a(1, 2, 3, 4);
		test(a[0], 1);
		test(a[1], 2);
		test(a[2], 3);
		test(a[3], 4);
		constexpr Vec4 b(3, 2, 4, 1);
		test(a, Vec4(1, 2, 3, 4));
		test(Vec4(3, 2, 4, 1), b);
		CAGE_TEST(a != b);
		constexpr Vec4 c = a + b;
		test(a + b, Vec4(4, 4, 7, 5));
		test(a + b, c);
		test(a * b, Vec4(3, 4, 12, 4));
	}

	void testMathIVec2()
	{
		CAGE_TESTCASE("ivec2");
		constexpr Vec2i a(3, 5);
		test(a[0], 3);
		test(a[1], 5);
		constexpr Vec2i b(2, 1);
		constexpr Vec2i c(5, 2);
		test(a, Vec2i(3, 5));
		CAGE_TEST(a != b);
		test(a + b, Vec2i(5, 6));
		test(a * b, Vec2i(6, 5));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, c), Vec2i(5, 5));
		test(min(a, c), Vec2i(3, 2));
		constexpr Vec2i d = a + c;
		test(d, Vec2i(8, 7));

		test(Vec2i(1, 5), Vec2i(1, 5));
		CAGE_TEST(!(Vec2i(1, 5) != Vec2i(1, 5)));
		CAGE_TEST(Vec2i(3, 5) != Vec2i(1, 5));
	}

	void testMathIVec3()
	{
		CAGE_TESTCASE("ivec3");
		constexpr Vec3i a(3, 5, 1);
		constexpr Vec3i b(1, 1, 4);
		test(a[0], 3);
		test(a[1], 5);
		test(a[2], 1);
		test(a, Vec3i(3, 5, 1));
		test(Vec3i(1, 1, 4), b);
		CAGE_TEST(a != b);
		test(a, a);
		test(a + b, Vec3i(4, 6, 5));
		test(a * b, Vec3i(3, 5, 4));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, b), Vec3i(3, 5, 4));
		test(min(a, b), Vec3i(1, 1, 1));
		constexpr Vec3i c = a + b;
		test(c, Vec3i(4, 6, 5));
		test(a / b, Vec3i(3, 5, 0));
		test(8 / b, Vec3i(8, 8, 2));
		test(a / 2, Vec3i(1, 2, 0));

		test(Vec3i(1, 5, 4), Vec3i(1, 5, 4));
		CAGE_TEST(!(Vec3i(1, 5, 4) != Vec3i(1, 5, 4)));
		CAGE_TEST(Vec3i(3, 5, 4) != Vec3i(1, 5, 4));

		CAGE_TEST(clamp(Vec3i(1, 3, 5), Vec3i(2), Vec3i(4)) == Vec3i(2, 3, 4));
		CAGE_TEST(clamp(Vec3i(1, 3, 5), 2, 4) == Vec3i(2, 3, 4));

		CAGE_TEST(abs(Vec3i(1, 3, 5)) == Vec3i(1, 3, 5));
		CAGE_TEST(abs(Vec3i(1, -3, 5)) == Vec3i(1, 3, 5));
		CAGE_TEST(abs(Vec3i(-1, 3, -5)) == Vec3i(1, 3, 5));
		CAGE_TEST(abs(Vec3i(-1, -3, -5)) == Vec3i(1, 3, 5));

	}

	void testMathIVec4()
	{
		CAGE_TESTCASE("ivec4");
		constexpr Vec4i a(1, 2, 3, 4);
		test(a[0], 1);
		test(a[1], 2);
		test(a[2], 3);
		test(a[3], 4);
		constexpr Vec4i b(3, 2, 4, 1);
		test(a, Vec4i(1, 2, 3, 4));
		test(Vec4i(3, 2, 4, 1), b);
		CAGE_TEST(a != b);
		constexpr Vec4i c = a + b;
		test(a + b, Vec4i(4, 4, 7, 5));
		test(a + b, c);
		test(a * b, Vec4i(3, 4, 12, 4));
	}

	void testMathQuat()
	{
		CAGE_TESTCASE("quat");

		{
			CAGE_TESTCASE("basic constructor");
			constexpr Quat q(1, 2, 3, 4);
			test(q[0], 1);
			test(q[1], 2);
			test(q[2], 3);
			test(q[3], 4);
		}

		{
			CAGE_TESTCASE("constructor from euler angles");
			test(Quat(Degs(), Degs(), Degs()), Quat(0, 0, 0, 1));
			test(Quat(Degs(90), Degs(), Degs()), Quat(0.7071067811865475, 0, 0, 0.7071067811865476));
			test(Quat(Degs(-90), Degs(), Degs()), Quat(-0.7071067811865475, 0, 0, 0.7071067811865476));
			test(Quat(Degs(), Degs(90), Degs()), Quat(0, 0.7071067811865475, 0, 0.7071067811865476));
			test(Quat(Degs(), Degs(-90), Degs()), Quat(0, -0.7071067811865475, 0, 0.7071067811865476));
			test(Quat(Degs(), Degs(), Degs(90)), Quat(0, 0, 0.7071067811865475, 0.7071067811865476));
			test(Quat(Degs(), Degs(), Degs(-90)), Quat(0, 0, -0.7071067811865475, 0.7071067811865476));
		}

		{
			CAGE_TESTCASE("multiplying quaternions");

			{
				CAGE_TESTCASE("multiplying by identity");
				test(Quat(0, 0, 0, 1) * Quat(0, 0, 0, 1), Quat(0, 0, 0, 1));
				test(Quat(0, 0, 1, 0) * Quat(0, 0, 0, 1), Quat(0, 0, 1, 0));
				test(Quat(0, 1, 0, 0) * Quat(0, 0, 0, 1), Quat(0, 1, 0, 0));
				test(Quat(1, 0, 0, 0) * Quat(0, 0, 0, 1), Quat(1, 0, 0, 0));
			}

			{
				CAGE_TESTCASE("x times x, ...");
				test(Quat(1, 0, 0, 0) * Quat(1, 0, 0, 0), Quat(0, 0, 0, -1));
				test(Quat(0, 1, 0, 0) * Quat(0, 1, 0, 0), Quat(0, 0, 0, -1));
				test(Quat(0, 0, 1, 0) * Quat(0, 0, 1, 0), Quat(0, 0, 0, -1));
			}

			{
				CAGE_TESTCASE("x times y, ...");
				test(Quat(1, 0, 0, 0) * Quat(0, 1, 0, 0), Quat(0, 0, 1, 0));
				test(Quat(0, 1, 0, 0) * Quat(0, 0, 1, 0), Quat(1, 0, 0, 0));
				test(Quat(0, 0, 1, 0) * Quat(1, 0, 0, 0), Quat(0, 1, 0, 0));
			}

			{
				CAGE_TESTCASE("reverse rotations");
				test(Quat(Degs(15), Degs(), Degs()) * Quat(Degs(-15), Degs(), Degs()), Quat());
				test(Quat(Degs(), Degs(15), Degs()) * Quat(Degs(), Degs(-15), Degs()), Quat());
				test(Quat(Degs(), Degs(), Degs(15)) * Quat(Degs(), Degs(), Degs(-15)), Quat());
				test(Quat(Degs(30), Degs(), Degs()) * Quat(Degs(-30), Degs(), Degs()), Quat());
				test(Quat(Degs(), Degs(30), Degs()) * Quat(Degs(), Degs(-30), Degs()), Quat());
				test(Quat(Degs(), Degs(), Degs(30)) * Quat(Degs(), Degs(), Degs(-30)), Quat());
				test(Quat(Degs(60), Degs(), Degs()) * Quat(Degs(-60), Degs(), Degs()), Quat());
				test(Quat(Degs(), Degs(60), Degs()) * Quat(Degs(), Degs(-60), Degs()), Quat());
				test(Quat(Degs(), Degs(), Degs(60)) * Quat(Degs(), Degs(), Degs(-60)), Quat());
				test(Quat(Degs(90), Degs(), Degs()) * Quat(Degs(-90), Degs(), Degs()), Quat());
				test(Quat(Degs(), Degs(90), Degs()) * Quat(Degs(), Degs(-90), Degs()), Quat());
				test(Quat(Degs(), Degs(), Degs(90)) * Quat(Degs(), Degs(), Degs(-90)), Quat());
			}

			{
				CAGE_TESTCASE("generic rotations");
				test(Quat(Degs(), Degs(30), Degs()) * Quat(Degs(), Degs(), Degs(20)), Quat(0.044943455527547777, 0.25488700224417876, 0.16773125949652062, 0.9512512425641977));
			}
		}

		{
			CAGE_TESTCASE("rotating vector");

			constexpr Vec3 vforward(0, 0, -10), vleft(-10, 0, 0), vright(10, 0, 0), vup(0, 10, 0), vdown(0, -10, 0), vback(0, 0, 10);

			{
				CAGE_TESTCASE("pitch");
				test(Quat(Degs(90), Degs(0), Degs(0)) * vforward, vup);
				test(Quat(Degs(90), Degs(0), Degs(0)) * vup, vback);
				test(Quat(Degs(90), Degs(0), Degs(0)) * vback, vdown);
				test(Quat(Degs(90), Degs(0), Degs(0)) * vdown, vforward);
				test(Quat(Degs(90), Degs(0), Degs(0)) * vleft, vleft);
				test(Quat(Degs(90), Degs(0), Degs(0)) * vright, vright);
				test(Quat(Degs(-90), Degs(0), Degs(0)) * vforward, vdown);
				test(Quat(Degs(-90), Degs(0), Degs(0)) * vback, vup);
			}

			{
				CAGE_TESTCASE("yaw");
				test(Quat(Degs(0), Degs(90), Degs(0)) * vleft, vback);
				test(Quat(Degs(0), Degs(90), Degs(0)) * vback, vright);
				test(Quat(Degs(0), Degs(90), Degs(0)) * vright, vforward);
				test(Quat(Degs(0), Degs(90), Degs(0)) * vup, vup);
				test(Quat(Degs(0), Degs(90), Degs(0)) * vdown, vdown);
				test(Quat(Degs(0), Degs(-90), Degs(0)) * vforward, vright);
				test(Quat(Degs(0), Degs(-90), Degs(0)) * vback, vleft);
			}

			{
				CAGE_TESTCASE("roll");
				test(Quat(Degs(0), Degs(0), Degs(90)) * vforward, vforward);
			}
			{
				CAGE_TESTCASE("commutativity");
				test(Quat(Degs(0), Degs(90), Degs(0)) * vforward, vleft);
				test(vforward * Quat(Degs(0), Degs(90), Degs(0)), vleft);
			}
		}

		{
			CAGE_TESTCASE("compare quaternions with lookAt");

			Mat4 q1 = Mat4(Quat());
			Mat4 m1 = lookAt(Vec3(), Vec3(0, 0, -1), Vec3(0, 1, 0));
			test(q1, m1);

			Mat4 q2 = Mat4(Quat(Degs(), Degs(-45), Degs()));
			Mat4 m2 = lookAt(Vec3(), normalize(Vec3(-1, 0, -1)), Vec3(0, 1, 0));
			test(q2, m2);

			Mat4 q3 = Mat4(Quat(Degs(-45), Degs(), Degs()));
			Mat4 m3 = lookAt(Vec3(), normalize(Vec3(0, 1, -1)), Vec3(0, 1, 0));
			test(q3, m3);
		}

		{
			CAGE_TESTCASE("quaternion from forward and up");

			{ // no rotation, keep forward
				Quat q(Vec3(0, 0, -1), Vec3(0, 1, 0), false);
				test(q * Vec3(0, 0, -1), Vec3(0, 0, -1));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(1, 0, 0));
			}

			{ // no rotation, keep up
				Quat q(Vec3(0, 0, -1), Vec3(0, 1, 0), true);
				test(q * Vec3(0, 0, -1), Vec3(0, 0, -1));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(1, 0, 0));
			}

			{ // rotation left, keep forward
				Quat q(Vec3(-1, 0, 0), Vec3(0, 1, 0), false);
				test(q * Vec3(0, 0, -1), Vec3(-1, 0, 0));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 0, -1));
			}

			{ // rotation left, keep up
				Quat q(Vec3(-1, 0, 0), Vec3(0, 1, 0), true);
				test(q * Vec3(0, 0, -1), Vec3(-1, 0, 0));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 0, -1));
			}

			{ // rotation right, keep forward
				Quat q(Vec3(1, 0, 0), Vec3(0, 1, 0), false);
				test(q * Vec3(0, 0, -1), Vec3(1, 0, 0));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 0, 1));
			}

			{ // rotation right, keep up
				Quat q(Vec3(1, 0, 0), Vec3(0, 1, 0), true);
				test(q * Vec3(0, 0, -1), Vec3(1, 0, 0));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 0, 1));
			}

			{ // rotation 180, keep forward
				Quat q(Vec3(0, 0, 1), Vec3(0, 1, 0), false);
				test(q * Vec3(0, 0, -1), Vec3(0, 0, 1));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(-1, 0, 0));
			}

			{ // rotation 180, keep up
				Quat q(Vec3(0, 0, 1), Vec3(0, 1, 0), true);
				test(q * Vec3(0, 0, -1), Vec3(0, 0, 1));
				test(q * Vec3(0, 1, 0), Vec3(0, 1, 0));
				test(q * Vec3(1, 0, 0), Vec3(-1, 0, 0));
			}

			{ // up is left, keep forward
				Quat q(Vec3(0, 0, -1), Vec3(-1, 0, 0), false);
				test(q * Vec3(0, 0, -1), Vec3(0, 0, -1));
				test(q * Vec3(0, 1, 0), Vec3(-1, 0, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 1, 0));
			}

			{ // up is left, keep up
				Quat q(Vec3(0, 0, -1), Vec3(-1, 0, 0), true);
				test(q * Vec3(0, 0, -1), Vec3(0, 0, -1));
				test(q * Vec3(0, 1, 0), Vec3(-1, 0, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 1, 0));
			}

			{ // uhh, keep forward
				Quat q(Vec3(0, 1, 0), Vec3(-1, 0, 0), false);
				test(q * Vec3(0, 0, -1), Vec3(0, 1, 0));
				test(q * Vec3(0, 1, 0), Vec3(-1, 0, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 0, 1));
			}

			{ // uhh, keep up
				Quat q(Vec3(0, 1, 0), Vec3(-1, 0, 0), true);
				test(q * Vec3(0, 0, -1), Vec3(0, 1, 0));
				test(q * Vec3(0, 1, 0), Vec3(-1, 0, 0));
				test(q * Vec3(1, 0, 0), Vec3(0, 0, 1));
			}

			// random
			for (uint32 i = 0; i < 10; i++)
			{
				const Vec3 f = randomDirection3();
				const Vec3 u = randomDirection3();
				const Vec3 r = normalize(cross(f, u));
				test(f, normalize(f));
				test(u, normalize(u));
				{ // keep forward
					Quat q(f, u, false);
					test(q * Vec3(0, 0, -1), f);
					test(q * Vec3(0, 1, 0), normalize(cross(r, f)));
					test(q * Vec3(1, 0, 0), r);
				}
				{ // keep up
					Quat q(f, u, true);
					test(q * Vec3(0, 0, -1), normalize(cross(u, r)));
					test(q * Vec3(0, 1, 0), u);
					test(q * Vec3(1, 0, 0), r);
				}
			}
		}

		{
			CAGE_TESTCASE("slerpPrecise");
			for (uint32 i = 0; i < 10; i++)
			{
				Quat q1 = randomDirectionQuat();
				Quat q2 = randomDirectionQuat();
				for (Real t = 0.1; t < 0.9; t += 0.1)
				{
					Quat qs = slerpPrecise(q1, q2, t);
					Quat qf = slerp(q1, q2, t);
					test(qs, qf);
				}
			}
		}

		{
			CAGE_TESTCASE("angle");
			{
				const Quat q1 = Quat(Vec3(0, 0, -1), Vec3(0, 1, 0));
				const Quat q2 = Quat(Vec3(0, 0, -1), Vec3(0, 1, 0));
				const Rads a = angle(q1, q2);
				test(a, Degs(0));
			}
			{
				const Quat q1 = Quat(Vec3(0, 0, -1), Vec3(0, 1, 0));
				const Quat q2 = Quat(Vec3(0, 0, -1), Vec3(1, 0, 0));
				const Rads a = angle(q1, q2);
				test(a, Degs(90));
			}
			{
				const Quat q1 = Quat(Vec3(0, 0, -1), Vec3(0, 1, 0));
				const Quat q2 = Quat(Vec3(1, 0, 0), Vec3(0, 1, 0));
				const Rads a = angle(q1, q2);
				test(a, Degs(90));
			}
			{
				const Quat q1 = Quat(Vec3(0, 0, -1), Vec3(0, 1, 0));
				const Quat q2 = Quat(Vec3(0, 0, 1), Vec3(0, 1, 0));
				const Rads a = angle(q1, q2);
				test(a, Degs(180));
			}
		}

		{
			CAGE_TESTCASE("quaternion to euler angles");

			for (uint32 i = 0; i < 100; i++)
			{
				const Quat a = randomDirectionQuat();
				const Rads p = pitch(a);
				const Rads y = yaw(a);
				const Rads r = roll(a);
				const Quat b = Quat(p, y, r);
				test(a, b);
			}
		}

		{
			CAGE_TESTCASE("vec3 *= quat");
			Vec3 a = Vec3(3, 4, 5);
			a *= Quat();
			CAGE_TEST(a == Vec3(3, 4, 5));
		}
	}

	void testMathMat3()
	{
		CAGE_TESTCASE("mat3");

		{
			CAGE_TESTCASE("constructor from basis vectors");

			{ // identity
				test(Mat3(Vec3(0, 0, -1), Vec3(0, 1, 0)), Mat3());
			}

			{ // turn left
				Mat3 m(Vec3(-1, 0, 0), Vec3(0, 1, 0));
				test(m * Vec3(0, 0, -1), Vec3(-1, 0, 0));
				test(m * Vec3(0, 1, 0), Vec3(0, 1, 0));
			}
		}

		{
			CAGE_TESTCASE("vec3 *= mat3");
			Vec3 a = Vec3(3, 4, 5);
			a *= Mat3(2, 0, 0, 0, 2, 0, 0, 0, 2);
			CAGE_TEST(a == Vec3(6, 8, 10));
		}
	}

	void testMathMat4()
	{
		CAGE_TESTCASE("mat4");

		{
			CAGE_TESTCASE("multiplication");

			constexpr Mat4 a(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 4, 5, 6, 1);
			constexpr Mat4 b(0, 0, 5, 0, 0, 1, 0, 0, -5, 0, 0, 0, 0, 0, 0, 1);
			constexpr Mat4 c(0, 0, 5, 0, 0, 1, 0, 0, -5, 0, 0, 0, -30, 5, 20, 1);
			Mat4 d = b * a;
			test(d, c);
		}

		{
			CAGE_TESTCASE("compose mat4 from position, rotation and scale");

			{
				constexpr Vec3 pos(5, 4, 3);
				Quat rot(Degs(), Degs(90), Degs());
				constexpr Vec3 scale(1, 1, 1);
				Mat4 m(pos, rot, scale);
				test(m * Vec4(0, 0, 0, 1), Vec4(5, 4, 3, 1));
				test(m * Vec4(2, 0, 0, 1), Vec4(5, 4, 1, 1));
				test(m * Vec4(0, 0, -1, 0), Vec4(-1, 0, 0, 0));
			}

			{
				constexpr Vec3 pos(5, 4, 3);
				Quat rot(Degs(), Degs(90), Degs());
				constexpr Vec3 scale(2, 2, 2);
				Mat4 m(pos, rot, scale);
				test(m * Vec4(0, 0, 0, 1), Vec4(5, 4, 3, 1));
				test(m * Vec4(2, 0, 0, 1), Vec4(5, 4, -1, 1));
				test(m * Vec4(0, 0, -1, 0), Vec4(-2, 0, 0, 0));
			}

			for (uint32 round = 0; round < 10; round++)
			{
				Vec3 pos = randomDirection3() * randomRange(Real(0.1), 10);
				Quat rot = randomDirectionQuat();
				Vec3 scl = randomRange3(Real(0.1), 10);
				Mat4 m1 = Mat4(pos) * Mat4(rot) * Mat4::scale(scl);
				Mat4 m2 = Mat4(pos, rot, scl);
				test(m1, m2);
			}
		}

		{
			CAGE_TESTCASE("inverse");

			constexpr Mat4 a(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 50, -20, 9, 1);
			Mat4 b = inverse(a);
			constexpr Mat4 c(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -50, 20, -9, 1);
			test(b, c);
		}

		{
			CAGE_TESTCASE("vec4 *= mat4");
			Vec4 a = Vec4(2, 3, 4, 5);
			a *= Mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2);
			CAGE_TEST(a == Vec4(4, 6, 8, 10));
		}
	}

	void testMathTransform()
	{
		CAGE_TESTCASE("transform");

		{
			CAGE_TESTCASE("basics");

			Transform t;
			Real r;
			Vec3 v;
			Quat q;
			Mat4 m;

			// todo check actual values

			t = t + v; // move transform
			t = t * q; // rotate transform
			t = t * r; // scale transform
			t = v + t;
			t = q * t;
			t = r * t;
			t += v;
			t *= q;
			t *= r;
			v = t * v; // transform vertex
			v = v * t;
		}

		{
			CAGE_TESTCASE("apply on vertex");

			for (uint32 round = 0; round < 10; round++)
			{
				Vec3 p = randomDirection3() * randomRange(-5, 20);
				Quat o = randomDirectionQuat();
				Real s = randomRange(Real(0.1), 10);
				Transform t(p, o, s);
				Mat4 m(t);
				Vec3 v = randomDirection3() * 10;
				Vec3 tv = t * v;
				Vec3 mv = Vec3(m * Vec4(v, 1));
				test(tv, mv);
			}
		}

		{
			CAGE_TESTCASE("concatenation");

			for (uint32 round = 0; round < 10; round++)
			{
				Vec3 pa = randomDirection3() * randomRange(-5, 20);
				Vec3 pb = randomDirection3() * randomRange(-5, 20);
				Quat oa = randomDirectionQuat();
				Quat ob = randomDirectionQuat();
				Real sa = randomRange(Real(0.1), 10);
				Real sb = randomRange(Real(0.1), 10);
				Transform ta(pa, oa, sa), tb(pb, ob, sb);
				test(Mat4(ta * tb), Mat4(ta) * Mat4(tb));
				interpolate(ta, tb, 0.42);
			}
		}

		{
			CAGE_TESTCASE("inverse");

			for (uint32 round = 0; round < 10; round++)
			{
				Vec3 p = randomDirection3() * randomRange(-5, 20);
				Quat o = randomDirectionQuat();
				Real s = randomRange(Real(0.1), 10);
				Transform t(p, o, s);
				Mat4 m1 = inverse(Mat4(t));
				Mat4 m2 = Mat4(inverse(t));
				test(m1, m2);
			}
		}
	}

	void testMathFunctions()
	{
		CAGE_TESTCASE("functions");

		{
			CAGE_TESTCASE("sin");
			test(sin(Rads(0)), 0);
			test(sin(Rads(Real::Pi() / 6)), ::sin(Real::Pi().value / 6));
			test(sin(Rads(Real::Pi() / 4)), ::sin(Real::Pi().value / 4));
			test(sin(Rads(Real::Pi() / 2)), ::sin(Real::Pi().value / 2));
			test(sin(Rads(Real::Pi() * 1)), ::sin(Real::Pi().value * 1));
			test(sin(Rads(Real::Pi() * 2)), ::sin(Real::Pi().value * 2));
		}

		{
			CAGE_TESTCASE("atan2");
			for (uint32 i = 0; i < 360; i++)
			{
				Rads ang = Degs(i);
				Real x = cos(ang), y = sin(ang);
				Rads ang2 = -atan2(x, -y); // this is insane
				test(ang2, ang);
			}
		}

		{
			CAGE_TESTCASE("random");
			for (uint32 i = 0; i < 1000; i++)
			{
				Real r = randomChance();
				CAGE_TEST(r >= 0 && r < 1, r);
			}
			for (uint32 i = 0; i < 1000; i++)
			{
				sint32 r = randomRange(5, 10);
				CAGE_TEST(r >= 5 && r < 10, r);
			}
			for (uint32 i = 0; i < 1000; i++)
			{
				sint32 r = randomRange(-10, 10);
				CAGE_TEST(r >= -10 && r < 10, r);
			}
		}
	}

	void testMathStrings()
	{
		CAGE_TESTCASE("math to string and back");
		{
			{
				CAGE_TESTCASE("real");
				for (uint32 i = 0; i < 10; i++)
				{
					Real v = randomRange(-1e5, 1e5);
					String s = Stringizer() + v;
					Real r = Real::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("rads");
				for (uint32 i = 0; i < 10; i++)
				{
					Rads v = Rads(randomRange(-1e5, 1e5));
					String s = Stringizer() + v;
					Rads r = Rads::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("degs");
				for (uint32 i = 0; i < 10; i++)
				{
					Degs v = Degs(randomRange(-1e5, 1e5));
					String s = Stringizer() + v;
					Degs r = Degs::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("vec2");
				for (uint32 i = 0; i < 10; i++)
				{
					Vec2 v = randomRange2(-1e5, 1e5);
					String s = Stringizer() + v;
					Vec2 r = Vec2::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("vec3");
				for (uint32 i = 0; i < 10; i++)
				{
					Vec3 v = randomRange3(-1e5, 1e5);
					String s = Stringizer() + v;
					Vec3 r = Vec3::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("vec4");
				for (uint32 i = 0; i < 10; i++)
				{
					Vec4 v = randomRange4(-1e5, 1e5);
					String s = Stringizer() + v;
					Vec4 r = Vec4::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("ivec2");
				for (uint32 i = 0; i < 10; i++)
				{
					Vec2i v = randomRange2i(-100000, 100000);
					String s = Stringizer() + v;
					Vec2i r = Vec2i::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("ivec3");
				for (uint32 i = 0; i < 10; i++)
				{
					Vec3i v = randomRange3i(-100000, 100000);
					String s = Stringizer() + v;
					Vec3i r = Vec3i::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("ivec4");
				for (uint32 i = 0; i < 10; i++)
				{
					Vec4i v = randomRange4i(-100000, 100000);
					String s = Stringizer() + v;
					Vec4i r = Vec4i::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("quat");
				for (uint32 i = 0; i < 10; i++)
				{
					Quat v = randomDirectionQuat();
					String s = Stringizer() + v;
					Quat r = Quat::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("mat3");
				for (uint32 i = 0; i < 10; i++)
				{
					Mat3 v = Mat3(randomDirectionQuat());
					String s = Stringizer() + v;
					Mat3 r = Mat3::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("mat4");
				for (uint32 i = 0; i < 10; i++)
				{
					Mat4 v = Mat4(randomDirection3() * 100, randomDirectionQuat(), randomDirection3() * 2);
					String s = Stringizer() + v;
					Mat4 r = Mat4::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("transform");
				for (uint32 i = 0; i < 10; i++)
				{
					Transform v = Transform(randomDirection3() * 100, randomDirectionQuat(), randomChance() * 2);
					String s = Stringizer() + v;
					// not yet implemented
					// transform r = transform::parse(s);
					// test((mat4)v, (mat4)r);
				}
			}
			{
				CAGE_TESTCASE("negative tests");
				CAGE_TEST_THROWN(Real::parse("bla"));
				CAGE_TEST_THROWN(Real::parse("(bla)"));
				CAGE_TEST_THROWN(Real::parse(""));
				CAGE_TEST_THROWN(Real::parse("  "));
				CAGE_TEST_THROWN(Real::parse("()"));
				CAGE_TEST_THROWN(Real::parse("-"));
				CAGE_TEST_THROWN(Real::parse("+"));
				CAGE_TEST_THROWN(Real::parse("(3"));
				CAGE_TEST_THROWN(Real::parse("3)"));
				CAGE_TEST_THROWN(Real::parse("1.0,"));
				CAGE_TEST_THROWN(Real::parse(",5"));
				CAGE_TEST_THROWN(Vec3::parse("bla"));
				CAGE_TEST_THROWN(Vec3::parse("(bla)"));
				CAGE_TEST_THROWN(Vec3::parse(""));
				CAGE_TEST_THROWN(Vec3::parse("()"));
				CAGE_TEST_THROWN(Vec3::parse("-"));
				CAGE_TEST_THROWN(Vec3::parse("+"));
				CAGE_TEST_THROWN(Vec3::parse("(3"));
				CAGE_TEST_THROWN(Vec3::parse("3)"));
				CAGE_TEST_THROWN(Vec3::parse("3,5"));
				CAGE_TEST_THROWN(Vec3::parse("3,"));
				CAGE_TEST_THROWN(Vec3::parse(",3"));
				CAGE_TEST_THROWN(Vec3::parse(",3,3"));
				CAGE_TEST_THROWN(Vec3::parse("3,3,"));
				CAGE_TEST_THROWN(Vec3::parse("3,3, "));
				CAGE_TEST_THROWN(Vec3::parse(" ,3, 4"));
				CAGE_TEST_THROWN(Vec3::parse("4 ,, 4"));
				CAGE_TEST_THROWN(Vec3::parse("4 , , 4"));
				CAGE_TEST_THROWN(Vec3::parse("4, ,5"));
				CAGE_TEST_THROWN(Vec3::parse("(4, ,5)"));
				CAGE_TEST_THROWN(Vec3i::parse("bla"));
				CAGE_TEST_THROWN(Vec3i::parse("(bla)"));
				CAGE_TEST_THROWN(Vec3i::parse(""));
				CAGE_TEST_THROWN(Vec3i::parse("()"));
				CAGE_TEST_THROWN(Vec3i::parse("-"));
				CAGE_TEST_THROWN(Vec3i::parse("+"));
				CAGE_TEST_THROWN(Vec3i::parse("(3"));
				CAGE_TEST_THROWN(Vec3i::parse("3)"));
				CAGE_TEST_THROWN(Vec3i::parse("3,5"));
				CAGE_TEST_THROWN(Vec3i::parse("3,"));
				CAGE_TEST_THROWN(Vec3i::parse(",3"));
				CAGE_TEST_THROWN(Vec3i::parse(",3,3"));
				CAGE_TEST_THROWN(Vec3i::parse("3,3,"));
				CAGE_TEST_THROWN(Vec3i::parse("3,3, "));
				CAGE_TEST_THROWN(Vec3i::parse(" ,3, 4"));
				CAGE_TEST_THROWN(Vec3i::parse("4 ,, 4"));
				CAGE_TEST_THROWN(Vec3i::parse("4 , , 4"));
				CAGE_TEST_THROWN(Vec3i::parse("4, ,5"));
				CAGE_TEST_THROWN(Vec3i::parse("(4, ,5)"));
			}
		}
	}

	void testMathMatrixMultiplication()
	{
		CAGE_TESTCASE("matrix multiplication - performance");

#if defined (CAGE_DEBUG)
		constexpr uint32 matricesCount = 100;
#else
		constexpr uint32 matricesCount = 10000;
#endif

		Mat4 matrices[matricesCount];
		for (uint32 i = 0; i < matricesCount; i++)
		{
			for (uint8 j = 0; j < 16; j++)
				matrices[i][j] = randomChance() * 100 - 50;
		}

		{
			CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "matrices count: " + matricesCount);
			Mat4 res;
			Holder<Timer> tmr = newTimer();
			for (uint32 i = 0; i < matricesCount; i++)
				res *= matrices[i];
			CAGE_LOG(SeverityEnum::Note, "test", Stringizer() + "duration: " + tmr->duration());
		}
	}
}

void testMath()
{
	CAGE_TESTCASE("math");
	testMathCompiles();
	testMathReal();
	testMathAngles();
	testMathVec2();
	testMathVec3();
	testMathVec4();
	testMathIVec2();
	testMathIVec3();
	testMathIVec4();
	testMathQuat();
	testMathMat3();
	testMathMat4();
	testMathTransform();
	testMathFunctions();
	testMathStrings();
	testMathMatrixMultiplication();
}

