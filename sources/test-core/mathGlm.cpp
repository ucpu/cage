#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "main.h"

#include <cage-core/camera.h>
#include <cage-core/math.h>

void test(Real a, Real b);
void test(Rads a, Rads b);
void test(const Quat &a, const Quat &b);
void test(const Vec3 &a, const Vec3 &b);
void test(const Vec4 &a, const Vec4 &b);
void test(const Mat3 &a, const Mat3 &b);
void test(const Mat4 &a, const Mat4 &b);

namespace
{
	const glm::vec3 c2g(const Vec3 v)
	{
		return glm::vec3(v.data[0].value, v.data[1].value, v.data[2].value);
	}
	const Vec3 g2c(const glm::vec3 v)
	{
		return Vec3(v.x, v.y, v.z);
	}
	const glm::quat c2g(const Quat v)
	{
		return glm::quat(v.data[3].value, v.data[0].value, v.data[1].value, v.data[2].value);
	}
	const Quat g2c(const glm::quat v)
	{
		return Quat(v.x, v.y, v.z, v.w);
	}
	const glm::mat3 c2g(const Mat3 &v)
	{
		return glm::mat3(v[0].value, v[1].value, v[2].value, v[3].value, v[4].value, v[5].value, v[6].value, v[7].value, v[8].value);
	}
	const Mat3 g2c(const glm::mat3 &v)
	{
		Mat3 r;
		for (int y = 0; y < 3; y++)
			for (int x = 0; x < 3; x++)
				r[y * 3 + x] = v[y][x];
		return r;
	}
	const glm::mat4 c2g(const Mat4 &v)
	{
		return glm::mat4(v[0].value, v[1].value, v[2].value, v[3].value, v[4].value, v[5].value, v[6].value, v[7].value, v[8].value, v[9].value, v[10].value, v[11].value, v[12].value, v[13].value, v[14].value, v[15].value);
	}
	const Mat4 g2c(const glm::mat4 &v)
	{
		Mat4 r;
		for (int y = 0; y < 4; y++)
			for (int x = 0; x < 4; x++)
				r[y * 4 + x] = v[y][x];
		return r;
	}
}

void testMathGlm()
{
	CAGE_TESTCASE("math with glm");
	{
		CAGE_TESTCASE("matrix glm casts");
		for (uint32 round = 0; round < 10; round++)
		{
			Mat3 m;
			for (uint32 i = 0; i < 9; i++)
				m[i] = randomChance() * 100;
			glm::mat3 mg = c2g(m);
			test(m, g2c(mg));
			CAGE_TEST(detail::memcmp(&m, glm::value_ptr(mg), sizeof(m)) == 0);
		}
		for (uint32 round = 0; round < 10; round++)
		{
			Mat4 m;
			for (uint32 i = 0; i < 16; i++)
				m[i] = randomChance() * 100;
			glm::mat4 mg = c2g(m);
			test(m, g2c(mg));
			CAGE_TEST(detail::memcmp(&m, glm::value_ptr(mg), sizeof(m)) == 0);
		}
	}
	{
		CAGE_TESTCASE("trigonometric functions");
		for (Rads a = Degs(0); a < Rads(Degs(360)); a += Degs(1))
		{
			test(sin(a), glm::sin(Real(a).value));
			test(cos(a), glm::cos(Real(a).value));
		}
		for (Real x = -10; x < 10; x += 0.1)
		{
			for (Real y = -10; y < 10; y += 0.1)
				if (x != 0 || y != 0)
					test(atan2(x, y), Rads(glm::atan(y.value, x.value)));
		}
	}
	{
		CAGE_TESTCASE("quaternion");
		{
			CAGE_TESTCASE("from angle axis");
			for (uint32 round = 0; round < 10; round++)
			{
				Rads a = randomAngle();
				Vec3 v = randomDirection3();
				Quat q = Quat(v, a);
				glm::quat g = glm::angleAxis(Real(a).value, c2g(v));
				test(q, g2c(g));
			}
		}
		{
			CAGE_TESTCASE("from euler");
			for (uint32 round = 0; round < 10; round++)
			{
				Rads pitch = randomAngle();
				Rads yaw = randomAngle();
				Rads roll = randomAngle();
				Quat q = Quat(pitch, yaw, roll);
				glm::quat g = glm::quat(c2g(Vec3(Real(pitch), Real(yaw), Real(roll))));
				test(q, g2c(g));
			}
		}
		{
			CAGE_TESTCASE("multiplication");
			for (uint32 round = 0; round < 10; round++)
			{
				Quat q;
				glm::quat g = { 1, 0, 0, 0 };
				for (int i = 0; i < 10; i++)
				{
					Quat q2 = randomDirectionQuat();
					glm::quat g2 = c2g(q2);
					q = normalize(q * q2);
					g = normalize(g * g2);
				}
				test(q, g2c(g));
			}
		}
		{
			CAGE_TESTCASE("rotating vector");
			for (uint32 round = 0; round < 10; round++)
			{
				Quat q = randomDirectionQuat();
				Vec3 v = randomDirection3() * randomRange(Real(0), 100);
				test(q * v, g2c(c2g(q) * c2g(v)));
			}
		}
		{
			CAGE_TESTCASE("slerp");
			for (uint32 round = 0; round < 10; round++)
			{
				Quat q1 = randomDirectionQuat();
				Quat q2 = randomDirectionQuat();
				Real f = randomChance();
				test(slerp(q1, q2, f), g2c(glm::slerp(c2g(q1), c2g(q2), f.value)));
			}
		}
		{
			CAGE_TESTCASE("dot");
			for (uint32 round = 0; round < 10; round++)
			{
				Quat q1 = randomDirectionQuat();
				Quat q2 = randomDirectionQuat();
				test(dot(q1, q2), glm::dot(c2g(q1), c2g(q2)));
			}
		}
		{
			CAGE_TESTCASE("matrix casts");
			for (uint32 round = 0; round < 10; round++)
			{
				Quat q = randomDirectionQuat();
				glm::quat g = c2g(q);
				Mat3 qm = Mat3(q);
				glm::mat3 gm = mat3_cast(g);
				test(qm, g2c(gm));
				test(q, Quat(Mat3(q)));
			}
		}
	}
	{
		CAGE_TESTCASE("matrix");
		{
			CAGE_TESTCASE("multiplication");
			for (uint32 round = 0; round < 10; round++)
			{
				Mat3 q;
				glm::mat3 g = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
				for (int i = 0; i < 10; i++)
				{
					Mat3 q2;
					for (uint32 i = 0; i < 9; i++)
						q2[i] = randomChance();
					glm::mat3 g2 = c2g(q2);
					q = q * q2;
					g = g * g2;
				}
				test(q, g2c(g));
			}
			for (uint32 round = 0; round < 10; round++)
			{
				Mat4 q;
				glm::mat4 g = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
				for (int i = 0; i < 10; i++)
				{
					Mat4 q2;
					for (uint32 i = 0; i < 16; i++)
						q2[i] = randomChance();
					glm::mat4 g2 = c2g(q2);
					q = q * q2;
					g = g * g2;
				}
				test(q, g2c(g));
			}
		}
		{
			CAGE_TESTCASE("determinant");
			for (uint32 round = 0; round < 10; round++)
			{
				Mat3 m;
				for (uint32 i = 0; i < 9; i++)
					m[i] = randomChance();
				Real mi = determinant(m);
				float gi = determinant(c2g(m));
				test(mi, Real(gi));
			}
			for (uint32 round = 0; round < 10; round++)
			{
				Mat4 m;
				for (uint32 i = 0; i < 16; i++)
					m[i] = randomChance();
				Real mi = determinant(m);
				float gi = determinant(c2g(m));
				test(mi, Real(gi));
			}
		}
		{
			CAGE_TESTCASE("inverse");
			for (uint32 round = 0; round < 10; round++)
			{
				Mat3 m = Mat3(randomDirectionQuat());
				Mat3 mi = inverse(m);
				Mat3 gi = g2c(inverse(c2g(m)));
				test(mi, gi);
			}
			for (uint32 round = 0; round < 10; round++)
			{
				Mat4 m = Mat4(randomChance3() * 2 - 1, randomDirectionQuat(), randomChance3() + 0.5);
				Mat4 mi = inverse(m);
				Mat4 gi = g2c(inverse(c2g(m)));
				test(mi, gi);
			}
		}
		{
			CAGE_TESTCASE("look at");
			for (uint32 round = 0; round < 10; round++)
			{
				Vec3 eye = randomDirection3() * randomRange(Real(1), 100);
				Vec3 dir = randomDirection3();
				Vec3 up = randomDirection3();
				while (abs(dot(dir, up)) > 0.999)
					up = randomDirection3(); // testing for correctness here, not for edge cases
				Mat4 c = lookAt(eye, eye + dir, up);
				Mat4 g = g2c(glm::lookAt(c2g(eye), c2g(eye + dir), c2g(up)));
				test(c, g);
			}
		}
		{
			CAGE_TESTCASE("projections");
			for (uint32 round = 0; round < 10; round++)
			{
				Real l = -randomRange(Real(1), 100);
				Real r = randomRange(Real(1), 100);
				Real b = -randomRange(Real(1), 100);
				Real t = randomRange(Real(1), 100);
				Real n = randomRange(Real(1), 100);
				Real f = n + randomRange(Real(1), 100);
				Mat4 c = perspectiveProjection(l, r, b, t, n, f);
				Mat4 g = g2c(glm::frustum(l.value, r.value, b.value, t.value, n.value, f.value));
				test(c, g);
			}
			for (uint32 round = 0; round < 10; round++)
			{
				Rads fov = randomRange(Degs(20), Degs(120));
				Real aspect = randomChance() * 2 + 0.1;
				Real n = randomRange(Real(1), 100);
				Real f = n + randomRange(Real(1), 100);
				Mat4 c = perspectiveProjection(fov, aspect, n, f);
				Mat4 g = g2c(glm::perspective(Real(fov).value, aspect.value, n.value, f.value));
				test(c, g);
				/*
				for (uint32 step = 0; step < 10; step++)
				{
					Vec3 a = randomRange3(-100, 100);
					const auto &proj = [&](Mat4 pr, Vec3 pt)
					{
						Vec4 k = pr * Vec4(pt, 1);
						return Vec3(k) / k[3];
					};
					Vec3 ac = proj(c, a);
					Vec3 ag = proj(g, a);
					test(ac, ag);
				}
				*/
			}
			for (uint32 round = 0; round < 10; round++)
			{
				Real l = -randomRange(Real(1), 100);
				Real r = randomRange(Real(1), 100);
				Real b = -randomRange(Real(1), 100);
				Real t = randomRange(Real(1), 100);
				Real n = randomRange(Real(1), 100);
				Real f = n + randomRange(Real(1), 100);
				Mat4 c = orthographicProjection(l, r, b, t, n, f);
				Mat4 g = g2c(glm::ortho(l.value, r.value, b.value, t.value, n.value, f.value));
				test(c, g);
			}
		}
		{
			CAGE_TESTCASE("matrix rotation");
			for (uint32 round = 0; round < 10; round++)
			{
				Rads rot = randomAngle();
				Mat4 m1 = Mat4(Quat(rot, Degs(), Degs()));
				glm::mat4 m2 = c2g(Mat4());
				m2 = glm::rotate(m2, Real(rot).value, glm::vec3(1, 0, 0));
				test(m1, g2c(m2));
			}
		}
		{
			CAGE_TESTCASE("compose matrix");
			for (uint32 round = 0; round < 10; round++)
			{
				Vec3 position = randomRange3(-50, 50);
				Rads rot = randomAngle();
				Vec3 scale = randomRange3(0.1, 10);
				Mat4 c = Mat4(position, Quat(rot, Degs(), Degs()), scale);
				glm::mat4 m = c2g(Mat4());
				m = glm::translate(m, c2g(position));
				if (round % 2)
					m = glm::rotate(m, Real(rot).value, glm::vec3(1, 0, 0));
				else
					m = m * c2g(Mat4(Quat(rot, Degs(), Degs())));
				m = glm::scale(m, c2g(scale));
				test(c, g2c(m));
			}
		}
	}
}
