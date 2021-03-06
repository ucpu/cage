#include "main.h"
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/timer.h>

#include <cmath>

void test(real a, real b)
{
	real d = abs(a - b);
	CAGE_TEST(d < 1e-4);
}
void test(sint32 a, sint32 b)
{
	CAGE_TEST(a == b);
}
void test(const mat4 &a, const mat4 &b)
{
	for (uint32 i = 0; i < 16; i++)
		test(a[i], b[i]);
}
void test(const mat3 &a, const mat3 &b)
{
	for (uint32 i = 0; i < 9; i++)
		test(a[i], b[i]);
}
void test(const vec4 &a, const vec4 &b)
{
	for (uint32 i = 0; i < 4; i++)
		test(a[i], b[i]);
}
void test(const vec3 &a, const vec3 &b)
{
	for (uint32 i = 0; i < 3; i++)
		test(a[i], b[i]);
}
void test(const vec2 &a, const vec2 &b)
{
	for (uint32 i = 0; i < 2; i++)
		test(a[i], b[i]);
}
void test(const ivec4 &a, const ivec4 &b)
{
	for (uint32 i = 0; i < 4; i++)
		test(a[i], b[i]);
}
void test(const ivec3 &a, const ivec3 &b)
{
	for (uint32 i = 0; i < 3; i++)
		test(a[i], b[i]);
}
void test(const ivec2 &a, const ivec2 &b)
{
	for (uint32 i = 0; i < 2; i++)
		test(a[i], b[i]);
}
void test(const quat &a, const quat &b)
{
	test(abs(dot(a, b)), 1);
	test(a * vec3(0, 0, 0.1), b * vec3(0, 0, 0.1));
	test(a * vec3(0, 0.1, 0), b * vec3(0, 0.1, 0));
	test(a * vec3(0.1, 0, 0), b * vec3(0.1, 0, 0));
}
void test(rads a, rads b)
{
	if (a < rads(0))
		a += rads::Full();
	if (b < rads(0))
		b += rads::Full();
	test(real(a), real(b));
}

namespace
{
	template<class T> struct MakeNotReal {};
	template<> struct MakeNotReal<vec2> { using type = ivec2; };
	template<> struct MakeNotReal<vec3> { using type = ivec3; };
	template<> struct MakeNotReal<vec4> { using type = ivec4; };
	template<> struct MakeNotReal<ivec2> { using type = vec2; };
	template<> struct MakeNotReal<ivec3> { using type = vec3; };
	template<> struct MakeNotReal<ivec4> { using type = vec4; };

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
			string s = stringizer() + a;
			T b = T::parse(s);
			CAGE_TEST(a == b);
		}

		{ // stringizer as l-Value
			T a;
			stringizer s;
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
			constexpr real k7 = lengthSquared(a);
			constexpr real k8 = dot(a, b);
			valid(a);
			constexpr T k9 = interpolate(a, b, 0.5);
			distance(a, b);
			constexpr real k10 = distanceSquared(a, b);
		}

		T t(3.5);
	}

	void testMathCompiles()
	{
		CAGE_TESTCASE("compile tests");
		checkVectorsCommon<vec2, 2>();
		checkVectorsCommon<vec3, 3>();
		checkVectorsCommon<vec4, 4>();
		checkVectorsCommon<ivec2, 2>();
		checkVectorsCommon<ivec3, 3>();
		checkVectorsCommon<ivec4, 4>();
		checkVectorsReal<vec2, 2>();
		checkVectorsReal<vec3, 3>();
		checkVectorsReal<vec4, 4>();

		{
			constexpr vec2 a = vec2(vec3(42));
			constexpr vec2 b = vec2(vec4(42));
			constexpr vec3 c = vec3(vec4(42));
			constexpr ivec2 d = ivec2(ivec3(42));
			constexpr ivec2 e = ivec2(ivec4(42));
			constexpr ivec3 f = ivec3(ivec4(42));
		}
	}

	void testMathReal()
	{
		CAGE_TESTCASE("real");

		{
			CAGE_TESTCASE("basics");
			CAGE_TEST(real::Pi() > 3.1 && real::Pi() < 3.2);
			CAGE_TEST(real::E() > 2.7 && real::E() < 2.8);
			CAGE_TEST(real(real::Infinity()).valid());
			CAGE_TEST(!real(real::Nan()).valid());
			CAGE_TEST(!real(real::Infinity()).finite());
			CAGE_TEST(!real(real::Nan()).finite());
			CAGE_TEST(real(real::Pi()).finite());
			CAGE_TEST(real(0).finite());
			CAGE_TEST(real(42).finite());
			CAGE_TEST(real(42).valid());
			constexpr real a = 3;
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
			test(real(+15) % real(+12), +15 % +12); // +3
			test(real(+15) % real(-12), +15 % -12); // +3
			test(real(-15) % real(-12), -15 % -12); // -3
			test(real(-15) % real(+12), -15 % +12); // -3
			test(real(+150) % real(+13), +150 % +13); // 7
			test(real(+150) % real(-13), +150 % -13); // 7
			test(real(-150) % real(-13), -150 % -13); // -7
			test(real(-150) % real(+13), -150 % +13); // -7
			test(real(+5) % real(+1.8), +1.4);
			test(real(+5) % real(-1.8), +1.4);
			test(real(-5) % real(-1.8), -1.4);
			test(real(-5) % real(+1.8), -1.4);
		}

		{
			CAGE_TESTCASE("wrap");
			test(wrap(0.9 * 15), real(0.9 * 15) % 1);
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
			test(interpolate(real(5), real(15), 0.5), 10);
			test(interpolate(real(5), real(15), 0), 5);
			test(interpolate(real(5), real(15), 1), 15);
			test(interpolate(real(5), real(15), 0.2), 7);
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
			test(saturate(4.5), (real)1.0);
			test(saturate(1.5), (real)1.0);
			test(saturate(0.65), (real)0.65);
			test(saturate(0.25), (real)0.25);
			test(saturate(-0.55), (real)0.0);
			test(saturate(-3.55), (real)0.0);
		}

		{
			CAGE_TESTCASE("rounding");
			CAGE_TEST(round(real(5.2)) == 5);
			CAGE_TEST(round(real(4.8)) == 5);
			CAGE_TEST(floor(real(5.2)) == 5);
			CAGE_TEST(floor(real(4.8)) == 4);
			CAGE_TEST(ceil(real(5.2)) == 6);
			CAGE_TEST(ceil(real(4.8)) == 5);
			CAGE_TEST(round(real(-5.2)) == -5);
			CAGE_TEST(round(real(-4.8)) == -5);
			CAGE_TEST(floor(real(-5.2)) == -6);
			CAGE_TEST(floor(real(-4.8)) == -5);
			CAGE_TEST(ceil(real(-5.2)) == -5);
			CAGE_TEST(ceil(real(-4.8)) == -4);
		}
	}

	void testMathAngles()
	{
		CAGE_TESTCASE("degs & rads");
		test(rads(degs(0)), rads(0));
		test(rads(degs(90)), rads(real::Pi() / 2));
		test(rads(degs(180)), rads(real::Pi()));
		test(rads(wrap(degs(90 * 15))), degs((90 * 15) % 360));
		test(rads(wrap(degs(0))), degs(0));
		test(rads(wrap(degs(-270))), degs(90));
		test(rads(wrap(degs(-180))), degs(180));
		test(rads(wrap(degs(-90))), degs(270));
		test(rads(wrap(degs(360))), degs(0));
		test(rads(wrap(degs(90))), degs(90));
		test(rads(wrap(degs(180))), degs(180));
		test(rads(wrap(degs(270))), degs(270));

		{
			CAGE_TESTCASE("operators");
			rads angle;
			real scalar;
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
				rads a = wrap(randomAngle());
				real d = randomRange(0.1, 10.0);
				real x = cos(a) * d;
				real y = sin(a) * d;
				rads r = wrap(atan2(x, y));
				test(a, r);
			}

			for (uint32 i = 0; i < 10; i++)
			{
				rads a = randomAngle();
				real d = randomRange(0.1, 10.0);
				real x = cos(a) * d;
				real y = sin(a) * d;
				rads c = atan2(x, y);
				rads s = rads(std::atan2(y.value, x.value));
				test(c, s);
			}
		}
	}

	void testMathVec2()
	{
		CAGE_TESTCASE("vec2");
		constexpr vec2 a(3, 5);
		test(a[0], 3);
		test(a[1], 5);
		constexpr vec2 b(2, 1);
		constexpr vec2 c(5, 2);
		test(a, vec2(3, 5));
		CAGE_TEST(a != b);
		test(a + b, vec2(5, 6));
		test(a * b, vec2(6, 5));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, c), vec2(5, 5));
		test(min(a, c), vec2(3, 2));
		constexpr vec2 d = a + c;
		test(d, vec2(8, 7));

		test(vec2(1, 5), vec2(1, 5));
		CAGE_TEST(!(vec2(1, 5) != vec2(1, 5)));
		CAGE_TEST(vec2(3, 5) != vec2(1, 5));
	}

	void testMathVec3()
	{
		CAGE_TESTCASE("vec3");
		constexpr vec3 a(3, 5, 1);
		constexpr vec3 b(1, 1, 4);
		test(a[0], 3);
		test(a[1], 5);
		test(a[2], 1);
		test(a, vec3(3, 5, 1));
		test(vec3(1, 1, 4), b);
		CAGE_TEST(a != b);
		test(a, a);
		test(a + b, vec3(4, 6, 5));
		test(a * b, vec3(3, 5, 4));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, b), vec3(3, 5, 4));
		test(min(a, b), vec3(1, 1, 1));
		constexpr vec3 c = a + b;
		test(c, vec3(4, 6, 5));
		test(cross(a, b), vec3(19, -11, -2));
		test(dot(a, b), 12);
		test(a / b, vec3(3, 5, 1.f / 4));
		test(8 / b, vec3(8, 8, 2));
		test(a / (1.f / 3), vec3(9, 15, 3));

		test(vec3(1, 5, 4), vec3(1, 5, 4));
		CAGE_TEST(!(vec3(1, 5, 4) != vec3(1, 5, 4)));
		CAGE_TEST(vec3(3, 5, 4) != vec3(1, 5, 4));

		CAGE_TEST(clamp(vec3(1, 3, 5), vec3(2), vec3(4)) == vec3(2, 3, 4));
		CAGE_TEST(clamp(vec3(1, 3, 5), 2, 4) == vec3(2, 3, 4));

		CAGE_TEST(abs(vec3(1, 3, 5)) == vec3(1, 3, 5));
		CAGE_TEST(abs(vec3(1, -3, 5)) == vec3(1, 3, 5));
		CAGE_TEST(abs(vec3(-1, 3, -5)) == vec3(1, 3, 5));
		CAGE_TEST(abs(vec3(-1, -3, -5)) == vec3(1, 3, 5));

		test(dominantAxis(vec3(13, -4, 1)), vec3(1, 0, 0));
		test(dominantAxis(vec3(-3, -15, 4)), vec3(0, -1, 0));

		// right = forward x up (in this order)
		test(cross(vec3(0, 0, -1), vec3(0, 1, 0)), vec3(1, 0, 0));
	}

	void testMathVec4()
	{
		CAGE_TESTCASE("vec4");
		constexpr vec4 a(1, 2, 3, 4);
		test(a[0], 1);
		test(a[1], 2);
		test(a[2], 3);
		test(a[3], 4);
		constexpr vec4 b(3, 2, 4, 1);
		test(a, vec4(1, 2, 3, 4));
		test(vec4(3, 2, 4, 1), b);
		CAGE_TEST(a != b);
		constexpr vec4 c = a + b;
		test(a + b, vec4(4, 4, 7, 5));
		test(a + b, c);
		test(a * b, vec4(3, 4, 12, 4));
	}

	void testMathIVec2()
	{
		CAGE_TESTCASE("ivec2");
		constexpr ivec2 a(3, 5);
		test(a[0], 3);
		test(a[1], 5);
		constexpr ivec2 b(2, 1);
		constexpr ivec2 c(5, 2);
		test(a, ivec2(3, 5));
		CAGE_TEST(a != b);
		test(a + b, ivec2(5, 6));
		test(a * b, ivec2(6, 5));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, c), ivec2(5, 5));
		test(min(a, c), ivec2(3, 2));
		constexpr ivec2 d = a + c;
		test(d, ivec2(8, 7));

		test(ivec2(1, 5), ivec2(1, 5));
		CAGE_TEST(!(ivec2(1, 5) != ivec2(1, 5)));
		CAGE_TEST(ivec2(3, 5) != ivec2(1, 5));
	}

	void testMathIVec3()
	{
		CAGE_TESTCASE("ivec3");
		constexpr ivec3 a(3, 5, 1);
		constexpr ivec3 b(1, 1, 4);
		test(a[0], 3);
		test(a[1], 5);
		test(a[2], 1);
		test(a, ivec3(3, 5, 1));
		test(ivec3(1, 1, 4), b);
		CAGE_TEST(a != b);
		test(a, a);
		test(a + b, ivec3(4, 6, 5));
		test(a * b, ivec3(3, 5, 4));
		test(a + b, b + a);
		test(a * b, b * a);
		test(max(a, b), ivec3(3, 5, 4));
		test(min(a, b), ivec3(1, 1, 1));
		constexpr ivec3 c = a + b;
		test(c, ivec3(4, 6, 5));
		test(a / b, ivec3(3, 5, 0));
		test(8 / b, ivec3(8, 8, 2));
		test(a / 2, ivec3(1, 2, 0));

		test(ivec3(1, 5, 4), ivec3(1, 5, 4));
		CAGE_TEST(!(ivec3(1, 5, 4) != ivec3(1, 5, 4)));
		CAGE_TEST(ivec3(3, 5, 4) != ivec3(1, 5, 4));

		CAGE_TEST(clamp(ivec3(1, 3, 5), ivec3(2), ivec3(4)) == ivec3(2, 3, 4));
		CAGE_TEST(clamp(ivec3(1, 3, 5), 2, 4) == ivec3(2, 3, 4));

		CAGE_TEST(abs(ivec3(1, 3, 5)) == ivec3(1, 3, 5));
		CAGE_TEST(abs(ivec3(1, -3, 5)) == ivec3(1, 3, 5));
		CAGE_TEST(abs(ivec3(-1, 3, -5)) == ivec3(1, 3, 5));
		CAGE_TEST(abs(ivec3(-1, -3, -5)) == ivec3(1, 3, 5));

	}

	void testMathIVec4()
	{
		CAGE_TESTCASE("ivec4");
		constexpr ivec4 a(1, 2, 3, 4);
		test(a[0], 1);
		test(a[1], 2);
		test(a[2], 3);
		test(a[3], 4);
		constexpr ivec4 b(3, 2, 4, 1);
		test(a, ivec4(1, 2, 3, 4));
		test(ivec4(3, 2, 4, 1), b);
		CAGE_TEST(a != b);
		constexpr ivec4 c = a + b;
		test(a + b, ivec4(4, 4, 7, 5));
		test(a + b, c);
		test(a * b, ivec4(3, 4, 12, 4));
	}

	void testMathQuat()
	{
		CAGE_TESTCASE("quat");

		{
			CAGE_TESTCASE("basic constructor");
			constexpr quat q(1, 2, 3, 4);
			test(q[0], 1);
			test(q[1], 2);
			test(q[2], 3);
			test(q[3], 4);
		}

		{
			CAGE_TESTCASE("constructor from euler angles");
			test(quat(degs(), degs(), degs()), quat(0, 0, 0, 1));
			test(quat(degs(90), degs(), degs()), quat(0.7071067811865475, 0, 0, 0.7071067811865476));
			test(quat(degs(-90), degs(), degs()), quat(-0.7071067811865475, 0, 0, 0.7071067811865476));
			test(quat(degs(), degs(90), degs()), quat(0, 0.7071067811865475, 0, 0.7071067811865476));
			test(quat(degs(), degs(-90), degs()), quat(0, -0.7071067811865475, 0, 0.7071067811865476));
			test(quat(degs(), degs(), degs(90)), quat(0, 0, 0.7071067811865475, 0.7071067811865476));
			test(quat(degs(), degs(), degs(-90)), quat(0, 0, -0.7071067811865475, 0.7071067811865476));
		}

		{
			CAGE_TESTCASE("multiplying quaternions");

			{
				CAGE_TESTCASE("multiplying by identity");
				test(quat(0, 0, 0, 1) * quat(0, 0, 0, 1), quat(0, 0, 0, 1));
				test(quat(0, 0, 1, 0) * quat(0, 0, 0, 1), quat(0, 0, 1, 0));
				test(quat(0, 1, 0, 0) * quat(0, 0, 0, 1), quat(0, 1, 0, 0));
				test(quat(1, 0, 0, 0) * quat(0, 0, 0, 1), quat(1, 0, 0, 0));
			}

			{
				CAGE_TESTCASE("x times x, ...");
				test(quat(1, 0, 0, 0) * quat(1, 0, 0, 0), quat(0, 0, 0, -1));
				test(quat(0, 1, 0, 0) * quat(0, 1, 0, 0), quat(0, 0, 0, -1));
				test(quat(0, 0, 1, 0) * quat(0, 0, 1, 0), quat(0, 0, 0, -1));
			}

			{
				CAGE_TESTCASE("x times y, ...");
				test(quat(1, 0, 0, 0) * quat(0, 1, 0, 0), quat(0, 0, 1, 0));
				test(quat(0, 1, 0, 0) * quat(0, 0, 1, 0), quat(1, 0, 0, 0));
				test(quat(0, 0, 1, 0) * quat(1, 0, 0, 0), quat(0, 1, 0, 0));
			}

			{
				CAGE_TESTCASE("reverse rotations");
				test(quat(degs(15), degs(), degs()) * quat(degs(-15), degs(), degs()), quat());
				test(quat(degs(), degs(15), degs()) * quat(degs(), degs(-15), degs()), quat());
				test(quat(degs(), degs(), degs(15)) * quat(degs(), degs(), degs(-15)), quat());
				test(quat(degs(30), degs(), degs()) * quat(degs(-30), degs(), degs()), quat());
				test(quat(degs(), degs(30), degs()) * quat(degs(), degs(-30), degs()), quat());
				test(quat(degs(), degs(), degs(30)) * quat(degs(), degs(), degs(-30)), quat());
				test(quat(degs(60), degs(), degs()) * quat(degs(-60), degs(), degs()), quat());
				test(quat(degs(), degs(60), degs()) * quat(degs(), degs(-60), degs()), quat());
				test(quat(degs(), degs(), degs(60)) * quat(degs(), degs(), degs(-60)), quat());
				test(quat(degs(90), degs(), degs()) * quat(degs(-90), degs(), degs()), quat());
				test(quat(degs(), degs(90), degs()) * quat(degs(), degs(-90), degs()), quat());
				test(quat(degs(), degs(), degs(90)) * quat(degs(), degs(), degs(-90)), quat());
			}

			{
				CAGE_TESTCASE("generic rotations");
				test(quat(degs(), degs(30), degs()) * quat(degs(), degs(), degs(20)), quat(0.044943455527547777, 0.25488700224417876, 0.16773125949652062, 0.9512512425641977));
			}
		}

		{
			CAGE_TESTCASE("rotating vector");

			constexpr vec3 vforward(0, 0, -10), vleft(-10, 0, 0), vright(10, 0, 0), vup(0, 10, 0), vdown(0, -10, 0), vback(0, 0, 10);

			{
				CAGE_TESTCASE("pitch");
				test(quat(degs(90), degs(0), degs(0)) * vforward, vup);
				test(quat(degs(90), degs(0), degs(0)) * vup, vback);
				test(quat(degs(90), degs(0), degs(0)) * vback, vdown);
				test(quat(degs(90), degs(0), degs(0)) * vdown, vforward);
				test(quat(degs(90), degs(0), degs(0)) * vleft, vleft);
				test(quat(degs(90), degs(0), degs(0)) * vright, vright);
				test(quat(degs(-90), degs(0), degs(0)) * vforward, vdown);
				test(quat(degs(-90), degs(0), degs(0)) * vback, vup);
			}

			{
				CAGE_TESTCASE("yaw");
				test(quat(degs(0), degs(90), degs(0)) * vleft, vback);
				test(quat(degs(0), degs(90), degs(0)) * vback, vright);
				test(quat(degs(0), degs(90), degs(0)) * vright, vforward);
				test(quat(degs(0), degs(90), degs(0)) * vup, vup);
				test(quat(degs(0), degs(90), degs(0)) * vdown, vdown);
				test(quat(degs(0), degs(-90), degs(0)) * vforward, vright);
				test(quat(degs(0), degs(-90), degs(0)) * vback, vleft);
			}

			{
				CAGE_TESTCASE("roll");
				test(quat(degs(0), degs(0), degs(90)) * vforward, vforward);
			}
			{
				CAGE_TESTCASE("commutativity");
				test(quat(degs(0), degs(90), degs(0)) * vforward, vleft);
				test(vforward * quat(degs(0), degs(90), degs(0)), vleft);
			}
		}

		{
			CAGE_TESTCASE("compare quaternions with lookAt");

			mat4 q1 = mat4(quat());
			mat4 m1 = lookAt(vec3(), vec3(0, 0, -1), vec3(0, 1, 0));
			test(q1, m1);

			mat4 q2 = mat4(quat(degs(), degs(-45), degs()));
			mat4 m2 = lookAt(vec3(), normalize(vec3(-1, 0, -1)), vec3(0, 1, 0));
			test(q2, m2);

			mat4 q3 = mat4(quat(degs(-45), degs(), degs()));
			mat4 m3 = lookAt(vec3(), normalize(vec3(0, 1, -1)), vec3(0, 1, 0));
			test(q3, m3);
		}

		{
			CAGE_TESTCASE("quaternion from forward and up");

			{ // no rotation, keep forward
				quat q(vec3(0, 0, -1), vec3(0, 1, 0), false);
				test(q * vec3(0, 0, -1), vec3(0, 0, -1));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(1, 0, 0));
			}

			{ // no rotation, keep up
				quat q(vec3(0, 0, -1), vec3(0, 1, 0), true);
				test(q * vec3(0, 0, -1), vec3(0, 0, -1));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(1, 0, 0));
			}

			{ // rotation left, keep forward
				quat q(vec3(-1, 0, 0), vec3(0, 1, 0), false);
				test(q * vec3(0, 0, -1), vec3(-1, 0, 0));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(0, 0, -1));
			}

			{ // rotation left, keep up
				quat q(vec3(-1, 0, 0), vec3(0, 1, 0), true);
				test(q * vec3(0, 0, -1), vec3(-1, 0, 0));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(0, 0, -1));
			}

			{ // rotation right, keep forward
				quat q(vec3(1, 0, 0), vec3(0, 1, 0), false);
				test(q * vec3(0, 0, -1), vec3(1, 0, 0));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(0, 0, 1));
			}

			{ // rotation right, keep up
				quat q(vec3(1, 0, 0), vec3(0, 1, 0), true);
				test(q * vec3(0, 0, -1), vec3(1, 0, 0));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(0, 0, 1));
			}

			{ // rotation 180, keep forward
				quat q(vec3(0, 0, 1), vec3(0, 1, 0), false);
				test(q * vec3(0, 0, -1), vec3(0, 0, 1));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(-1, 0, 0));
			}

			{ // rotation 180, keep up
				quat q(vec3(0, 0, 1), vec3(0, 1, 0), true);
				test(q * vec3(0, 0, -1), vec3(0, 0, 1));
				test(q * vec3(0, 1, 0), vec3(0, 1, 0));
				test(q * vec3(1, 0, 0), vec3(-1, 0, 0));
			}

			{ // up is left, keep forward
				quat q(vec3(0, 0, -1), vec3(-1, 0, 0), false);
				test(q * vec3(0, 0, -1), vec3(0, 0, -1));
				test(q * vec3(0, 1, 0), vec3(-1, 0, 0));
				test(q * vec3(1, 0, 0), vec3(0, 1, 0));
			}

			{ // up is left, keep up
				quat q(vec3(0, 0, -1), vec3(-1, 0, 0), true);
				test(q * vec3(0, 0, -1), vec3(0, 0, -1));
				test(q * vec3(0, 1, 0), vec3(-1, 0, 0));
				test(q * vec3(1, 0, 0), vec3(0, 1, 0));
			}

			{ // uhh, keep forward
				quat q(vec3(0, 1, 0), vec3(-1, 0, 0), false);
				test(q * vec3(0, 0, -1), vec3(0, 1, 0));
				test(q * vec3(0, 1, 0), vec3(-1, 0, 0));
				test(q * vec3(1, 0, 0), vec3(0, 0, 1));
			}

			{ // uhh, keep up
				quat q(vec3(0, 1, 0), vec3(-1, 0, 0), true);
				test(q * vec3(0, 0, -1), vec3(0, 1, 0));
				test(q * vec3(0, 1, 0), vec3(-1, 0, 0));
				test(q * vec3(1, 0, 0), vec3(0, 0, 1));
			}

			// random
			for (uint32 i = 0; i < 10; i++)
			{
				const vec3 f = randomDirection3();
				const vec3 u = randomDirection3();
				const vec3 r = normalize(cross(f, u));
				test(f, normalize(f));
				test(u, normalize(u));
				{ // keep forward
					quat q(f, u, false);
					test(q * vec3(0, 0, -1), f);
					test(q * vec3(0, 1, 0), normalize(cross(r, f)));
					test(q * vec3(1, 0, 0), r);
				}
				{ // keep up
					quat q(f, u, true);
					test(q * vec3(0, 0, -1), normalize(cross(u, r)));
					test(q * vec3(0, 1, 0), u);
					test(q * vec3(1, 0, 0), r);
				}
			}
		}

		{
			CAGE_TESTCASE("slerpPrecise");
			for (uint32 i = 0; i < 10; i++)
			{
				quat q1 = randomDirectionQuat();
				quat q2 = randomDirectionQuat();
				for (real t = 0.1; t < 0.9; t += 0.1)
				{
					quat qs = slerpPrecise(q1, q2, t);
					quat qf = slerp(q1, q2, t);
					test(qs, qf);
				}
			}
		}

		{
			CAGE_TESTCASE("angle");
			{
				const quat q1 = quat(vec3(0, 0, -1), vec3(0, 1, 0));
				const quat q2 = quat(vec3(0, 0, -1), vec3(0, 1, 0));
				const rads a = angle(q1, q2);
				test(a, degs(0));
			}
			{
				const quat q1 = quat(vec3(0, 0, -1), vec3(0, 1, 0));
				const quat q2 = quat(vec3(0, 0, -1), vec3(1, 0, 0));
				const rads a = angle(q1, q2);
				test(a, degs(90));
			}
			{
				const quat q1 = quat(vec3(0, 0, -1), vec3(0, 1, 0));
				const quat q2 = quat(vec3(1, 0, 0), vec3(0, 1, 0));
				const rads a = angle(q1, q2);
				test(a, degs(90));
			}
			{
				const quat q1 = quat(vec3(0, 0, -1), vec3(0, 1, 0));
				const quat q2 = quat(vec3(0, 0, 1), vec3(0, 1, 0));
				const rads a = angle(q1, q2);
				test(a, degs(180));
			}
		}

		{
			CAGE_TESTCASE("quaternion to euler angles");

			for (uint32 i = 0; i < 100; i++)
			{
				const quat a = randomDirectionQuat();
				const rads p = pitch(a);
				const rads y = yaw(a);
				const rads r = roll(a);
				const quat b = quat(p, y, r);
				test(a, b);
			}
		}
	}

	void testMathMat3()
	{
		CAGE_TESTCASE("mat3");

		{
			CAGE_TESTCASE("constructor from basis vectors");

			{ // identity
				test(mat3(vec3(0, 0, -1), vec3(0, 1, 0)), mat3());
			}

			{ // turn left
				mat3 m(vec3(-1, 0, 0), vec3(0, 1, 0));
				test(m * vec3(0, 0, -1), vec3(-1, 0, 0));
				test(m * vec3(0, 1, 0), vec3(0, 1, 0));
			}
		}
	}

	void testMathMat4()
	{
		CAGE_TESTCASE("mat4");

		{
			CAGE_TESTCASE("multiplication");

			constexpr mat4 a(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 4, 5, 6, 1);
			constexpr mat4 b(0, 0, 5, 0, 0, 1, 0, 0, -5, 0, 0, 0, 0, 0, 0, 1);
			constexpr mat4 c(0, 0, 5, 0, 0, 1, 0, 0, -5, 0, 0, 0, -30, 5, 20, 1);
			mat4 d = b * a;
			test(d, c);
		}

		{
			CAGE_TESTCASE("compose mat4 from position, rotation and scale");

			{
				constexpr vec3 pos(5, 4, 3);
				quat rot(degs(), degs(90), degs());
				constexpr vec3 scale(1, 1, 1);
				mat4 m(pos, rot, scale);
				test(m * vec4(0, 0, 0, 1), vec4(5, 4, 3, 1));
				test(m * vec4(2, 0, 0, 1), vec4(5, 4, 1, 1));
				test(m * vec4(0, 0, -1, 0), vec4(-1, 0, 0, 0));
			}

			{
				constexpr vec3 pos(5, 4, 3);
				quat rot(degs(), degs(90), degs());
				constexpr vec3 scale(2, 2, 2);
				mat4 m(pos, rot, scale);
				test(m * vec4(0, 0, 0, 1), vec4(5, 4, 3, 1));
				test(m * vec4(2, 0, 0, 1), vec4(5, 4, -1, 1));
				test(m * vec4(0, 0, -1, 0), vec4(-2, 0, 0, 0));
			}

			for (uint32 round = 0; round < 10; round++)
			{
				vec3 pos = randomDirection3() * randomRange(real(0.1), 10);
				quat rot = randomDirectionQuat();
				vec3 scl = randomRange3(real(0.1), 10);
				mat4 m1 = mat4(pos) * mat4(rot) * mat4::scale(scl);
				mat4 m2 = mat4(pos, rot, scl);
				test(m1, m2);
			}
		}

		{
			CAGE_TESTCASE("inverse");

			constexpr mat4 a(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 50, -20, 9, 1);
			mat4 b = inverse(a);
			constexpr mat4 c(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -50, 20, -9, 1);
			test(b, c);
		}
	}

	void testMathTransform()
	{
		CAGE_TESTCASE("transform");

		{
			CAGE_TESTCASE("basics");

			transform t;
			real r;
			vec3 v;
			quat q;
			mat4 m;

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
				vec3 p = randomDirection3() * randomRange(-5, 20);
				quat o = randomDirectionQuat();
				real s = randomRange(real(0.1), 10);
				transform t(p, o, s);
				mat4 m(t);
				vec3 v = randomDirection3() * 10;
				vec3 tv = t * v;
				vec3 mv = vec3(m * vec4(v, 1));
				test(tv, mv);
			}
		}

		{
			CAGE_TESTCASE("concatenation");

			for (uint32 round = 0; round < 10; round++)
			{
				vec3 pa = randomDirection3() * randomRange(-5, 20);
				vec3 pb = randomDirection3() * randomRange(-5, 20);
				quat oa = randomDirectionQuat();
				quat ob = randomDirectionQuat();
				real sa = randomRange(real(0.1), 10);
				real sb = randomRange(real(0.1), 10);
				transform ta(pa, oa, sa), tb(pb, ob, sb);
				test(mat4(ta * tb), mat4(ta) * mat4(tb));
				interpolate(ta, tb, 0.42);
			}
		}

		{
			CAGE_TESTCASE("inverse");

			for (uint32 round = 0; round < 10; round++)
			{
				vec3 p = randomDirection3() * randomRange(-5, 20);
				quat o = randomDirectionQuat();
				real s = randomRange(real(0.1), 10);
				transform t(p, o, s);
				mat4 m1 = inverse(mat4(t));
				mat4 m2 = mat4(inverse(t));
				test(m1, m2);
				round = round;
			}
		}
	}

	void testMathFunctions()
	{
		CAGE_TESTCASE("functions");

		{
			CAGE_TESTCASE("sin");
			test(sin(rads(0)), 0);
			test(sin(rads(real::Pi() / 6)), ::sin(real::Pi().value / 6));
			test(sin(rads(real::Pi() / 4)), ::sin(real::Pi().value / 4));
			test(sin(rads(real::Pi() / 2)), ::sin(real::Pi().value / 2));
			test(sin(rads(real::Pi() * 1)), ::sin(real::Pi().value * 1));
			test(sin(rads(real::Pi() * 2)), ::sin(real::Pi().value * 2));
		}

		{
			CAGE_TESTCASE("atan2");
			for (uint32 i = 0; i < 360; i++)
			{
				rads ang = degs(i);
				real x = cos(ang), y = sin(ang);
				rads ang2 = -atan2(x, -y); // this is insane
				test(ang2, ang);
			}
		}

		{
			CAGE_TESTCASE("random");
			for (uint32 i = 0; i < 1000; i++)
			{
				real r = randomChance();
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
					real v = randomRange(-1e5, 1e5);
					string s = stringizer() + v;
					real r = real::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("rads");
				for (uint32 i = 0; i < 10; i++)
				{
					rads v = rads(randomRange(-1e5, 1e5));
					string s = stringizer() + v;
					rads r = rads::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("degs");
				for (uint32 i = 0; i < 10; i++)
				{
					degs v = degs(randomRange(-1e5, 1e5));
					string s = stringizer() + v;
					degs r = degs::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("vec2");
				for (uint32 i = 0; i < 10; i++)
				{
					vec2 v = randomRange2(-1e5, 1e5);
					string s = stringizer() + v;
					vec2 r = vec2::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("vec3");
				for (uint32 i = 0; i < 10; i++)
				{
					vec3 v = randomRange3(-1e5, 1e5);
					string s = stringizer() + v;
					vec3 r = vec3::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("vec4");
				for (uint32 i = 0; i < 10; i++)
				{
					vec4 v = randomRange4(-1e5, 1e5);
					string s = stringizer() + v;
					vec4 r = vec4::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("ivec2");
				for (uint32 i = 0; i < 10; i++)
				{
					ivec2 v = randomRange2i(-100000, 100000);
					string s = stringizer() + v;
					ivec2 r = ivec2::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("ivec3");
				for (uint32 i = 0; i < 10; i++)
				{
					ivec3 v = randomRange3i(-100000, 100000);
					string s = stringizer() + v;
					ivec3 r = ivec3::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("ivec4");
				for (uint32 i = 0; i < 10; i++)
				{
					ivec4 v = randomRange4i(-100000, 100000);
					string s = stringizer() + v;
					ivec4 r = ivec4::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("quat");
				for (uint32 i = 0; i < 10; i++)
				{
					quat v = randomDirectionQuat();
					string s = stringizer() + v;
					quat r = quat::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("mat3");
				for (uint32 i = 0; i < 10; i++)
				{
					mat3 v = mat3(randomDirectionQuat());
					string s = stringizer() + v;
					mat3 r = mat3::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("mat4");
				for (uint32 i = 0; i < 10; i++)
				{
					mat4 v = mat4(randomDirection3() * 100, randomDirectionQuat(), randomDirection3() * 2);
					string s = stringizer() + v;
					mat4 r = mat4::parse(s);
					test(v, r);
				}
			}
			{
				CAGE_TESTCASE("transform");
				for (uint32 i = 0; i < 10; i++)
				{
					transform v = transform(randomDirection3() * 100, randomDirectionQuat(), randomChance() * 2);
					string s = stringizer() + v;
					// not yet implemented
					// transform r = transform::parse(s);
					// test((mat4)v, (mat4)r);
				}
			}
			{
				CAGE_TESTCASE("negative tests");
				CAGE_TEST_THROWN(real::parse("bla"));
				CAGE_TEST_THROWN(real::parse("(bla)"));
				CAGE_TEST_THROWN(real::parse(""));
				CAGE_TEST_THROWN(real::parse("  "));
				CAGE_TEST_THROWN(real::parse("()"));
				CAGE_TEST_THROWN(real::parse("-"));
				CAGE_TEST_THROWN(real::parse("+"));
				CAGE_TEST_THROWN(real::parse("(3"));
				CAGE_TEST_THROWN(real::parse("3)"));
				CAGE_TEST_THROWN(real::parse("1.0,"));
				CAGE_TEST_THROWN(real::parse(",5"));
				CAGE_TEST_THROWN(vec3::parse("bla"));
				CAGE_TEST_THROWN(vec3::parse("(bla)"));
				CAGE_TEST_THROWN(vec3::parse(""));
				CAGE_TEST_THROWN(vec3::parse("()"));
				CAGE_TEST_THROWN(vec3::parse("-"));
				CAGE_TEST_THROWN(vec3::parse("+"));
				CAGE_TEST_THROWN(vec3::parse("(3"));
				CAGE_TEST_THROWN(vec3::parse("3)"));
				CAGE_TEST_THROWN(vec3::parse("3,5"));
				CAGE_TEST_THROWN(vec3::parse("3,"));
				CAGE_TEST_THROWN(vec3::parse(",3"));
				CAGE_TEST_THROWN(vec3::parse(",3,3"));
				CAGE_TEST_THROWN(vec3::parse("3,3,"));
				CAGE_TEST_THROWN(vec3::parse("3,3, "));
				CAGE_TEST_THROWN(vec3::parse(" ,3, 4"));
				CAGE_TEST_THROWN(vec3::parse("4 ,, 4"));
				CAGE_TEST_THROWN(vec3::parse("4 , , 4"));
				CAGE_TEST_THROWN(vec3::parse("4, ,5"));
				CAGE_TEST_THROWN(vec3::parse("(4, ,5)"));
				CAGE_TEST_THROWN(ivec3::parse("bla"));
				CAGE_TEST_THROWN(ivec3::parse("(bla)"));
				CAGE_TEST_THROWN(ivec3::parse(""));
				CAGE_TEST_THROWN(ivec3::parse("()"));
				CAGE_TEST_THROWN(ivec3::parse("-"));
				CAGE_TEST_THROWN(ivec3::parse("+"));
				CAGE_TEST_THROWN(ivec3::parse("(3"));
				CAGE_TEST_THROWN(ivec3::parse("3)"));
				CAGE_TEST_THROWN(ivec3::parse("3,5"));
				CAGE_TEST_THROWN(ivec3::parse("3,"));
				CAGE_TEST_THROWN(ivec3::parse(",3"));
				CAGE_TEST_THROWN(ivec3::parse(",3,3"));
				CAGE_TEST_THROWN(ivec3::parse("3,3,"));
				CAGE_TEST_THROWN(ivec3::parse("3,3, "));
				CAGE_TEST_THROWN(ivec3::parse(" ,3, 4"));
				CAGE_TEST_THROWN(ivec3::parse("4 ,, 4"));
				CAGE_TEST_THROWN(ivec3::parse("4 , , 4"));
				CAGE_TEST_THROWN(ivec3::parse("4, ,5"));
				CAGE_TEST_THROWN(ivec3::parse("(4, ,5)"));
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

		mat4 matrices[matricesCount];
		for (uint32 i = 0; i < matricesCount; i++)
		{
			for (uint8 j = 0; j < 16; j++)
				matrices[i][j] = randomChance() * 100 - 50;
		}

		{
			CAGE_LOG(SeverityEnum::Info, "test", stringizer() + "matrices count: " + matricesCount);
			mat4 res;
			Holder<Timer> tmr = newTimer();
			for (uint32 i = 0; i < matricesCount; i++)
				res *= matrices[i];
			CAGE_LOG(SeverityEnum::Note, "test", stringizer() + "duration: " + tmr->duration());
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

