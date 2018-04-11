#include "main.h"
#include <cage-core/math.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

void test(real a, real b);
void test(rads a, rads b);
void test(const quat &a, const quat &b);
void test(const vec3 &a, const vec3 &b);
void test(const vec4 &a, const vec4 &b);
void test(const mat3 &a, const mat3 &b);
void test(const mat4 &a, const mat4 &b);

namespace
{
	const glm::vec3 c2g(const vec3 v)
	{
		return glm::vec3(v.data[0].value, v.data[1].value, v.data[2].value);
	}
	const vec3 g2c(const glm::vec3 v)
	{
		return vec3(v.x, v.y, v.z);
	}
	const glm::quat c2g(const quat v)
	{
		return glm::quat(v.data[3].value, v.data[0].value, v.data[1].value, v.data[2].value);
	}
	const quat g2c(const glm::quat v)
	{
		return quat(v.x, v.y, v.z, v.w);
	}
	const glm::mat3 c2g(const mat3 &v)
	{
		return glm::mat3(
			v[0].value, v[1].value, v[2].value,
			v[3].value, v[4].value, v[5].value,
			v[6].value, v[7].value, v[8].value
		);
	}
	const mat3 g2c(const glm::mat3 &v)
	{
		mat3 r;
		for (int y = 0; y < 3; y++)
			for (int x = 0; x < 3; x++)
				r[y * 3 + x] = v[y][x];
		return r;
	}
	const glm::mat4 c2g(const mat4 &v)
	{
		return glm::mat4(
			v[0].value, v[1].value, v[2].value, v[3].value,
			v[4].value, v[5].value, v[6].value, v[7].value,
			v[8].value, v[9].value, v[10].value, v[11].value,
			v[12].value, v[13].value, v[14].value, v[15].value
		);
	}
	const mat4 g2c(const glm::mat4 &v)
	{
		mat4 r;
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
			mat3 m;
			for (uint32 i = 0; i < 9; i++)
				m[i] = cage::random();
			glm::mat3 mg = c2g(m);
			test(m, g2c(mg));
			CAGE_TEST(detail::memcmp(&m, glm::value_ptr(mg), sizeof(m)) == 0);
		}
		for (uint32 round = 0; round < 10; round++)
		{
			mat4 m;
			for (uint32 i = 0; i < 16; i++)
				m[i] = cage::random();
			glm::mat4 mg = c2g(m);
			test(m, g2c(mg));
			CAGE_TEST(detail::memcmp(&m, glm::value_ptr(mg), sizeof(m)) == 0);
		}
	}
	{
		CAGE_TESTCASE("trigonometric functions");
		for (rads a = degs(0); a < rads(degs(360)); a += degs(1))
		{
			test(sin(a), glm::sin(real(a).value));
			test(cos(a), glm::cos(real(a).value));
		}
		for (real x = -10; x < 10; x += 0.1)
		{
			for (real y = -10; y < 10; y += 0.1)
				if (x != 0 || y != 0)
					test(aTan2(x, y), rads(glm::atan(y.value, x.value)));
		}
	}
	{
		CAGE_TESTCASE("quaternion");
		{
			CAGE_TESTCASE("from angle axis");
			for (uint32 round = 0; round < 10; round++)
			{
				rads a = randomAngle();
				vec3 v = randomDirection3();
				quat q = quat(v, a);
				glm::quat g = glm::angleAxis(real(a).value, c2g(v));
				test(q, g2c(g));
			}
		}
		{
			CAGE_TESTCASE("from euler");
			for (uint32 round = 0; round < 10; round++)
			{
				rads pitch = randomAngle();
				rads yaw = randomAngle();
				rads roll = randomAngle();
				quat q = quat(pitch, yaw, roll);
				glm::quat g = glm::quat(c2g(vec3(real(pitch), real(yaw), real(roll))));
				test(q, g2c(g));
			}
		}
		{
			CAGE_TESTCASE("multiplication");
			for (uint32 round = 0; round < 10; round++)
			{
				quat q;
				glm::quat g = { 1, 0, 0, 0 };
				for (int i = 0; i < 10; i++)
				{
					quat q2 = randomDirectionQuat();
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
				quat q = randomDirectionQuat();
				vec3 v = randomDirection3() * random(real(0), 100);
				test(q * v, g2c(c2g(q) * c2g(v)));
			}
		}
		{
			CAGE_TESTCASE("slerp");
			for (uint32 round = 0; round < 10; round++)
			{
				quat q1 = randomDirectionQuat();
				quat q2 = randomDirectionQuat();
				real f = cage::random();
				test(slerp(q1, q2, f), g2c(glm::slerp(c2g(q1), c2g(q2), f.value)));
			}
		}
		{
			CAGE_TESTCASE("dot");
			for (uint32 round = 0; round < 10; round++)
			{
				quat q1 = randomDirectionQuat();
				quat q2 = randomDirectionQuat();
				test(dot(q1, q2), glm::dot(c2g(q1), c2g(q2)));
			}
		}
		{
			CAGE_TESTCASE("matrix casts");
			for (uint32 round = 0; round < 10; round++)
			{
				quat q = randomDirectionQuat();
				glm::quat g = c2g(q);
				mat3 qm = mat3(q);
				glm::mat3 gm = mat3_cast(g);
				test(qm, g2c(gm));
				test(q, quat(mat3(q)));
			}
		}
	}
	{
		CAGE_TESTCASE("matrix");
		{
			CAGE_TESTCASE("multiplication");
			for (uint32 round = 0; round < 10; round++)
			{
				mat3 q;
				glm::mat3 g = {1,0,0,0,1,0,0,0,1};
				for (int i = 0; i < 10; i++)
				{
					mat3 q2;
					for (uint32 i = 0; i < 9; i++)
						q2[i] = cage::random();
					glm::mat3 g2 = c2g(q2);
					q = q * q2;
					g = g * g2;
				}
				test(q, g2c(g));
			}
			for (uint32 round = 0; round < 10; round++)
			{
				mat4 q;
				glm::mat4 g = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
				for (int i = 0; i < 10; i++)
				{
					mat4 q2;
					for (uint32 i = 0; i < 16; i++)
						q2[i] = cage::random();
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
				mat3 m;
				for (uint32 i = 0; i < 9; i++)
					m[i] = cage::random();
				real mi = m.determinant();
				float gi = determinant(c2g(m));
				test(mi, real(gi));
			}
			for (uint32 round = 0; round < 10; round++)
			{
				mat4 m;
				for (uint32 i = 0; i < 16; i++)
					m[i] = cage::random();
				real mi = m.determinant();
				float gi = determinant(c2g(m));
				test(mi, real(gi));
			}
		}
		{
			CAGE_TESTCASE("inverse");
			for (uint32 round = 0; round < 10; round++)
			{
				mat3 m;
				for (uint32 i = 0; i < 9; i++)
					m[i] = cage::random();
				mat3 mi = m.inverse();
				mat3 gi = g2c(inverse(c2g(m)));
				for (uint32 i = 0; i < 9; i++)
					CAGE_TEST(abs(mi[i] - gi[i]) < abs(mi[i]) * 0.01);
			}
			for (uint32 round = 0; round < 10; round++)
			{
				mat4 m;
				for (uint32 i = 0; i < 16; i++)
					m[i] = cage::random();
				mat4 mi = m.inverse();
				mat4 gi = g2c(inverse(c2g(m)));
				for (uint32 i = 0; i < 16; i++)
					CAGE_TEST(abs(mi[i] - gi[i]) < abs(mi[i]) * 0.01);
			}
		}
		{
			CAGE_TESTCASE("look at");
			for (uint32 round = 0; round < 10; round++)
			{
				vec3 eye = randomDirection3() * random(real(1), 100);
				vec3 dir = randomDirection3();
				vec3 up = randomDirection3();
				mat4 c = lookAt(eye, eye + dir, up);
				mat4 g = g2c(glm::lookAt(c2g(eye), c2g(eye + dir), c2g(up)));
				test(c, g);
			}
		}
		{
			CAGE_TESTCASE("projections");
			for (uint32 round = 0; round < 10; round++)
			{
				real l = -random(real(1), 100);
				real r = random(real(1), 100);
				real b = -random(real(1), 100);
				real t = random(real(1), 100);
				real n = random(real(1), 100);
				real f = n + random(real(1), 100);
				mat4 c = perspectiveProjection(l, r, b, t, n, f);
				mat4 g = g2c(glm::frustum(l.value, r.value, b.value, t.value, n.value, f.value));
				test(c, g);
			}
			for (uint32 round = 0; round < 10; round++)
			{
				rads fov = random(rads(degs(20)), degs(120));
				real aspect = cage::random();
				real n = random(real(1), 100);
				real f = n + random(real(1), 100);
				mat4 c = perspectiveProjection(fov, aspect, n, f);
				mat4 g = g2c(glm::perspective(real(fov).value, aspect.value, n.value, f.value));
				test(c, g);
			}
			for (uint32 round = 0; round < 10; round++)
			{
				real l = -random(real(1), 100);
				real r = random(real(1), 100);
				real b = -random(real(1), 100);
				real t = random(real(1), 100);
				real n = random(real(1), 100);
				real f = n + random(real(1), 100);
				mat4 c = orthographicProjection(l, r, b, t, n, f);
				mat4 g = g2c(glm::ortho(l.value, r.value, b.value, t.value, n.value, f.value));
				test(c, g);
			}
		}
		{
			CAGE_TESTCASE("matrix rotation");
			for (uint32 round = 0; round < 10; round++)
			{
				rads rot = randomAngle();
				mat4 m1 = mat4(quat(rot, degs(), degs()));
				glm::mat4 m2 = c2g(mat4());
				m2 = glm::rotate(m2, real(rot).value, glm::vec3(1,0,0));
				test(m1, g2c(m2));
			}
		}
		{
			CAGE_TESTCASE("compose matrix");
			for (uint32 round = 0; round < 10; round++)
			{
				vec3 position = vec3(random(-50, 50), random(-50, 50), random(-50, 50));
				rads rot = randomAngle();
				vec3 scale = vec3(random(real(0.1), 10), random(real(0.1), 10), random(real(0.1), 10));
				mat4 c = mat4(position, quat(rot, degs(), degs()), scale);
				glm::mat4 m = c2g(mat4());
				m = glm::translate(m, c2g(position));
				if (round % 2)
					m = glm::rotate(m, real(rot).value, glm::vec3(1, 0, 0));
				else
					m = m * c2g(mat4(quat(rot, degs(), degs())));
				m = glm::scale(m, c2g(scale));
				test(c, g2c(m));
			}
		}
	}
}
