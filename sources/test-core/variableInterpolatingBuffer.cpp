#include "main.h"

#include <cage-core/math.h>
#include <cage-core/utility/variableInterpolatingBuffer.h>

void testVariableInterpolatingBuffer()
{
	CAGE_TESTCASE("variableInterpolatingBuffer");

	{
		CAGE_TESTCASE("compilations");

		variableInterpolatingBufferStruct<real> iReal;
		variableInterpolatingBufferStruct<degs> iDegs;
		variableInterpolatingBufferStruct<rads> iRads;
		variableInterpolatingBufferStruct<vec2> iVec2;
		variableInterpolatingBufferStruct<vec3> iVec3;
		variableInterpolatingBufferStruct<vec4> iVec4;
		variableInterpolatingBufferStruct<quat> iQuat;
		variableInterpolatingBufferStruct<mat3> iMat3;
		variableInterpolatingBufferStruct<mat4> iMat4;
	}

	{
		CAGE_TESTCASE("simple interpolations on real");

		variableInterpolatingBufferStruct<real> v;
		v.set(100, 100, 50);
		CAGE_TEST(v(50) == 100);
		CAGE_TEST(v(150) == 100);
		v.set(200, 200, 100);
		CAGE_TEST(v(50) == 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(130) == 130);
		CAGE_TEST(v(180) == 180);
		CAGE_TEST(v(200) == 200);
		v.set(300, 300, 100);
		CAGE_TEST(v(50) == 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(130) == 130);
		CAGE_TEST(v(180) == 180);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(230) == 230);
		CAGE_TEST(v(280) == 280);
		CAGE_TEST(v(300) == 300);
		v.set(400, 400, 200);
		CAGE_TEST(v(100) == 200);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(230) == 230);
		CAGE_TEST(v(280) == 280);
		CAGE_TEST(v(300) == 300);
		CAGE_TEST(v(330) == 330);
		CAGE_TEST(v(380) == 380);
		CAGE_TEST(v(400) == 400);
	}

	{
		CAGE_TESTCASE("advanced interpolations on real");

		variableInterpolatingBufferStruct<real> v;
		v.set(100, 100, 100);
		v.set(200, 200, 100);
		v.set(300, 300, 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(300) == 300);
		v.set(400, 400, 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(300) == 300);
		CAGE_TEST(v(400) == 400);
		v.set(500, 500, 500);
		CAGE_TEST(v(400) == 500);
		CAGE_TEST(v(600) == 500);
		v.set(600, 600, 600);
		CAGE_TEST(v(500) == 600);
		CAGE_TEST(v(700) == 600);
		v.set(800, 800, 700);
		CAGE_TEST(v(700) == 600);
		CAGE_TEST(v(800) == 800);
		v.set(900, 900, 700);
		CAGE_TEST(v(700) == 600);
		CAGE_TEST(v(800) == 800);
		CAGE_TEST(v(900) == 900);

		v.clear();
		v.set(100, 100, 100);
		v.set(200, 200, 100);
		v.set(300, 300, 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(300) == 300);
		v.set(150, 150, 200);
		CAGE_TEST(v(100) == 200);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(300) == 300);

		v.clear();
		v.set(100, 100, 100);
		v.set(200, 200, 100);
		v.set(300, 300, 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(300) == 300);
		v.set(400, 400, 300);
		CAGE_TEST(v(200) == 300);
		CAGE_TEST(v(300) == 300);
		CAGE_TEST(v(400) == 400);

		v.clear();
		v.set(100, 100, 100);
		v.set(300, 300, 100);
		v.set(500, 500, 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(300) == 300);
		CAGE_TEST(v(500) == 500);
		v.set(200, 200, 200);
		CAGE_TEST(v(100) == 200);
		CAGE_TEST(v(200) == 200);
		CAGE_TEST(v(300) == 300);
		CAGE_TEST(v(400) == 400);
		CAGE_TEST(v(500) == 500);

		v.clear();
		v.set(100, 100, 100);
		v.set(300, 300, 100);
		v.set(500, 500, 100);
		CAGE_TEST(v(100) == 100);
		CAGE_TEST(v(300) == 300);
		CAGE_TEST(v(500) == 500);
		v.set(400, 400, 400);
		CAGE_TEST(v(300) == 400);
		CAGE_TEST(v(400) == 400);
		CAGE_TEST(v(500) == 500);
	}
}
