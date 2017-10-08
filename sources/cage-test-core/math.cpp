#include <cmath>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/utility/timer.h>

void test(const mat4 &a, const mat4 &b)
{
	for (uint32 i = 0; i < 16; i++)
		CAGE_TEST(abs(a[i] - b[i]) < 1e-3);
}
void test(const mat3 &a, const mat3 &b)
{
	for (uint32 i = 0; i < 9; i++)
		CAGE_TEST(abs(a[i] - b[i]) < 1e-3);
}
void test(const vec4 &a, const vec4 &b)
{
	for (uint32 i = 0; i < 4; i++)
		CAGE_TEST(abs(a[i] - b[i]) < 1e-3);
}
void test(const vec3 &a, const vec3 &b)
{
	for (uint32 i = 0; i < 3; i++)
		CAGE_TEST(abs(a[i] - b[i]) < 1e-3);
}
void test(const vec2 &a, const vec2 &b)
{
	for (uint32 i = 0; i < 2; i++)
		CAGE_TEST(abs(a[i] - b[i]) < 1e-3);
}
void test(const quat &a, const quat &b)
{
	if (a.data[3] < 0) return test(inverse(a), b);
	if (b.data[3] < 0) return test(a, inverse(b));
	for (uint32 i = 0; i < 4; i++)
		CAGE_TEST(abs(a[i] - b[i]) < 1e-3);
}
void test(rads a, rads b)
{
	if (a < rads(0))
		a += rads::Full;
	if (b < rads(0))
		b += rads::Full;
	CAGE_TEST(abs(a - b) < rads(degs(1)));
}
void test(real a, real b)
{
	CAGE_TEST(abs(a - b) < 1e-3);
}

namespace
{
	template<class T, uint32 N> void checkVectors()
	{
		{
			T r;
			T a;
			const T b;
			r = a;
			r = b;
#define GCHL_GENERATE(OPERATOR) \
			r = b OPERATOR b; \
			r = b OPERATOR 5; \
			r = 5 OPERATOR b; \
			r = b OPERATOR 5.5f; \
			r = 5.5f OPERATOR b; \
			r = b OPERATOR 5.5; \
			r = 5.5 OPERATOR b;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, / ));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
			r = a OPERATOR b; \
			r = a OPERATOR 5; \
			r = a OPERATOR 5.5f; \
			r = a OPERATOR 5.f;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +=, -=, *=, /=));
#undef GCHL_GENERATE
			- b;
			+b;
			for (uint32 i = 0; i < N; i++)
				r[i] = b[i];
			CAGE_TEST(b == b);
			CAGE_TEST(!(b != b));
			string s(b);
			CAGE_TEST(s == s);
		}

		{
			const T a;
			const T b;
			a.inverse();
			a.normalize();
			a.min(b);
			a.max(b);
			a.min(0.5);
			a.max(0.5);
			a.clamp(a, b);
			a.clamp(0.1, 0.9);
			a.length();
			a.squaredLength();
			a.dot(b);
			a.valid();

			inverse(a);
			normalize(a);
			min(a, b);
			max(a, b);
			min(a, 0.5);
			max(a, 0.5);
			min(0.5, a);
			max(0.5, a);
			clamp(a, b, b);
			clamp(a, 0.1, 0.9);
			length(a);
			squaredLength(a);
			dot(a, b);
			valid(a);
			interpolate(a, b, 0.5);
		}
	}
}

void testMath()
{
	CAGE_TESTCASE("math");

	{
		CAGE_TESTCASE("compile tests");
		checkVectors<vec2, 2>();
		checkVectors<vec3, 3>();
		checkVectors<vec4, 4>();
	}

	{
		CAGE_TESTCASE("real");
		CAGE_TEST(real::Pi > 3.1 && real::Pi < 3.2);
		CAGE_TEST(real::E > 2.7 && real::E < 2.8);
		CAGE_TEST(real::HalfPi > 1.5 && real::HalfPi < 1.6);
		CAGE_TEST(real::TwoPi > 6.2 && real::TwoPi < 6.3);
		CAGE_TEST(real::PositiveInfinity.valid());
		CAGE_TEST(real::NegativeInfinity.valid());
		CAGE_TEST(!real::Nan.valid());
		CAGE_TEST(!real::PositiveInfinity.finite());
		CAGE_TEST(!real::NegativeInfinity.finite());
		CAGE_TEST(!real::Nan.finite());
		CAGE_TEST(real::Pi.finite());
		CAGE_TEST(real(0).finite());
		real a = 3;
		test(a + 2, 5);
		test(2 + a, 5);
		test(a - 2, 1);
		test(2 - a, -1);
		test(a * 2, 6);
		test(2 * a, 6);
		test(a / 2, 1.5);
		test(2 / a, 2.f / 3.f);

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
			CAGE_TESTCASE("interpolating real");
			test(interpolate(real(5), real(15), 0.5), 10);
			test(interpolate(real(5), real(15), 0), 5);
			test(interpolate(real(5), real(15), 1), 15);
			test(interpolate(real(5), real(15), 0.2), 7);
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

	{
		CAGE_TESTCASE("degs & rads");
		test(rads(degs(0)), rads(0));
		test(rads(degs(90)), rads(real::HalfPi));
		test(rads(degs(180)), rads(real::Pi));
		test(rads(degs(90 * 15)).normalize(), degs((90 * 15) % 360));
		test(rads(degs(0)).normalize(), degs(0));
		test(rads(degs(-270)).normalize(), degs(90));
		test(rads(degs(-180)).normalize(), degs(180));
		test(rads(degs(-90)).normalize(), degs(270));
		test(rads(degs(360)).normalize(), degs(0));
		test(rads(degs(90)).normalize(), degs(90));
		test(rads(degs(180)).normalize(), degs(180));
		test(rads(degs(270)).normalize(), degs(270));

		{
			CAGE_TESTCASE("operators");
			rads angle;
			real scalar;
			angle = angle + angle;
			angle = angle - angle;
			angle = angle * scalar;
			angle = scalar * angle;
			angle = angle / scalar;
			scalar = angle / angle;
		}
	}

	{
		CAGE_TESTCASE("vec2");
		vec2 a(3, 5);
		vec2 b(2, 1);
		vec2 c(5, 2);
		test(a, vec2(3, 5));
		CAGE_TEST(a != b);
		test(a + b, vec2(5, 6));
		test(a * b, vec2(6, 5));
		test(a + b, b + a);
		test(a * b, b * a);
		test(a.max(c), vec2(5, 5));
		test(a.min(c), vec2(3, 2));
		vec2 d = a + c;
		test(d, vec2(8, 7));

		test(vec2(1, 5), vec2(1, 5));
		CAGE_TEST(!(vec2(1, 5) != vec2(1, 5)));
		CAGE_TEST(vec2(3, 5) != vec2(1, 5));
	}

	{
		CAGE_TESTCASE("vec3");
		vec3 a(3, 5, 1);
		vec3 b(1, 1, 4);
		test(a, vec3(3, 5, 1));
		test(vec3(1, 1, 4), b);
		CAGE_TEST(a != b);
		test(a, a);
		test(a + b, vec3(4, 6, 5));
		test(a * b, vec3(3, 5, 4));
		test(a + b, b + a);
		test(a * b, b * a);
		test(a.max(b), vec3(3, 5, 4));
		test(a.min(b), vec3(1, 1, 1));
		vec3 c = a + b;
		test(c, vec3(4, 6, 5));
		test(a.cross(b), vec3(19, -11, -2));
		test(a.dot(b), 12);
		test(a / b, vec3(3, 5, 1.f / 4));
		test(8 / b, vec3(8, 8, 2));
		test(a / (1.f / 3), vec3(9, 15, 3));

		test(vec3(1, 5, 4), vec3(1, 5, 4));
		CAGE_TEST(!(vec3(1, 5, 4) != vec3(1, 5, 4)));
		CAGE_TEST(vec3(3, 5, 4) != vec3(1, 5, 4));

		CAGE_TEST(clamp(vec3(1, 3, 5), vec3(2, 2, 2), vec3(4, 4, 4)) == vec3(2, 3, 4));
		CAGE_TEST(clamp(vec3(1, 3, 5), 2, 4) == vec3(2, 3, 4));

		// right = forward x up (in this order)
		test(cross(vec3(0, 0, -1), vec3(0, 1, 0)), vec3(1, 0, 0));
	}

	{
		CAGE_TESTCASE("vec4");
		vec4 a(1, 2, 3, 4);
		vec4 b(3, 2, 4, 1);
		test(a, vec4(1, 2, 3, 4));
		test(vec4(3, 2, 4, 1), b);
		CAGE_TEST(a != b);
		vec4 c = a + b;
		test(a + b, vec4(4, 4, 7, 5));
		test(a + b, c);
		test(a * b, vec4(3, 4, 12, 4));
	}

	{
		CAGE_TESTCASE("quat");

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

			const vec3 vforward(0, 0, -10), vleft(-10, 0, 0), vright(10, 0, 0), vup(0, 10, 0), vdown(0, -10, 0), vback(0, 0, 10);

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
			mat4 m2 = lookAt(vec3(), vec3(-1, 0, -1).normalize(), vec3(0, 1, 0));
			test(q2, m2);

			mat4 q3 = mat4(quat(degs(-45), degs(), degs()));
			mat4 m3 = lookAt(vec3(), vec3(0, 1, -1).normalize(), vec3(0, 1, 0));
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
	}

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

	{
		CAGE_TESTCASE("mat4");

		mat4 a(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 4, 5, 6, 1);
		mat4 b(0, 0, 5, 0, 0, 1, 0, 0, -5, 0, 0, 0, 0, 0, 0, 1);
		mat4 c(0, 0, 5, 0, 0, 1, 0, 0, -5, 0, 0, 0, -30, 5, 20, 1);
		mat4 d = b * a;
		test(d, c);

		{
			CAGE_TESTCASE("compose mat4 from position, rotation and scale");

			{
				vec3 pos(5, 4, 3);
				quat rot(degs(), degs(90), degs());
				vec3 scale(1, 1, 1);
				mat4 m(pos, rot, scale);
				test(m * vec4(0, 0, 0, 1), vec4(5, 4, 3, 1));
				test(m * vec4(2, 0, 0, 1), vec4(5, 4, 1, 1));
				test(m * vec4(0, 0, -1, 0), vec4(-1, 0, 0, 0));
			}

			{
				vec3 pos(5, 4, 3);
				quat rot(degs(), degs(90), degs());
				vec3 scale(2, 2, 2);
				mat4 m(pos, rot, scale);
				test(m * vec4(0, 0, 0, 1), vec4(5, 4, 3, 1));
				test(m * vec4(2, 0, 0, 1), vec4(5, 4, -1, 1));
				test(m * vec4(0, 0, -1, 0), vec4(-2, 0, 0, 0));
			}

			for (uint32 round = 0; round < 10; round++)
			{
				vec3 pos = randomDirection3() * random(real(0.1), 10);
				quat rot = randomDirectionQuat();
				real scl = random(real(0.1), 10);
				mat4 m1 = mat4(pos) * mat4(rot) * mat4(scl);
				mat4 m2 = mat4(pos, rot, vec3(scl, scl, scl));
				test(m1, m2);
			}
		}
	}

	{
		CAGE_TESTCASE("transform");

		{
			CAGE_TESTCASE("concatenation");

			for (uint32 round = 0; round < 10; round++)
			{
				vec3 pa = randomDirection3() * random(-5, 20);
				vec3 pb = randomDirection3() * random(-5, 20);
				quat oa = randomDirectionQuat();
				quat ob = randomDirectionQuat();
				real sa = random(real(0.1), 10);
				real sb = random(real(0.1), 10);
				transform ta(pa, oa, sa), tb(pb, ob, sb);
				mat4 ma(pa, oa, vec3(sa, sa, sa)), mb(pb, ob, vec3(sb, sb, sb));
				test(mat4(ta), ma);
				mat4 mc1 = ma * mb;
				transform tc = ta * tb;
				mat4 mc2 = mat4(tc);
				test(mc1, mc2);
				interpolate(ta, tb, 0.42);
			}
		}

		{
			CAGE_TESTCASE("inverse");

			for (uint32 round = 0; round < 10; round++)
			{
				vec3 p = randomDirection3() * random(-5, 20);
				quat o = randomDirectionQuat();
				real s = random(real(0.1), 10);
				transform t(p, o, s);
				transform ti = t.inverse();
				mat4 m1 = mat4(t).inverse();
				mat4 m2 = mat4(ti);
				test(m1, m2);
			}
		}
	}

	{
		CAGE_TESTCASE("functions");

		{
			CAGE_TESTCASE("sin");
			test(sin(rads(0)), 0);
			test(sin(rads(real::Pi / 6)), ::sin(real::Pi.value / 6));
			test(sin(rads(real::Pi / 4)), ::sin(real::Pi.value / 4));
			test(sin(rads(real::Pi / 2)), ::sin(real::Pi.value / 2));
			test(sin(rads(real::Pi)), ::sin(real::Pi.value));
			test(sin(rads(real::Pi * 2)), ::sin(real::Pi.value * 2));
		}

		{
			CAGE_TESTCASE("atan2");
			for (uint32 i = 0; i < 360; i++)
			{
				rads ang = degs(i);
				real x = cos(ang), y = sin(ang);
				rads ang2 = -aTan2(x, -y); // this is insane
				test(ang2, ang);
			}
		}

		{
			CAGE_TESTCASE("random");
			for (uint32 i = 0; i < 1000; i++)
			{
				real r = random();
				CAGE_TEST(r >= 0 && r < 1, r);
			}
			for (uint32 i = 0; i < 1000; i++)
			{
				sint32 r = random(5, 10);
				CAGE_TEST(r >= 5 && r < 10, r);
			}
			for (uint32 i = 0; i < 1000; i++)
			{
				sint32 r = random(-10, 10);
				CAGE_TEST(r >= -10 && r < 10, r);
			}
		}
	}

	{
		CAGE_TESTCASE("matrix multiplication - performace");

#if defined (CAGE_DEBUG)
		static const uint32 matricesCount = 100;
#else
		static const uint32 matricesCount = 10000;
#endif

		mat4 matrices[matricesCount];
		for (uint32 i = 0; i < matricesCount; i++)
		{
			for (uint8 j = 0; j < 16; j++)
				matrices[i][j] = random() * 100 - 50;
		}

		{
			CAGE_LOG(severityEnum::Info, "test", string("matrices count: ") + matricesCount);
			mat4 res;
			holder <timerClass> tmr = newTimer();
			for (uint32 i = 0; i < matricesCount; i++)
				res *= matrices[i];
			CAGE_LOG(severityEnum::Note, "test", string("duration: ") + tmr->microsSinceStart());
		}
	}
}
