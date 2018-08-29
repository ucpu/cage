#include "main.h"

#include <cage-core/math.h>
#include <cage-core/utility/variableSmoothingBuffer.h>

void testVariableSmoothingBufferStruct()
{
	CAGE_TESTCASE("variableSmoothingBuffer");

	{
		CAGE_TESTCASE("compilations");

		variableSmoothingBufferStruct<real> iReal;
		variableSmoothingBufferStruct<degs> iDegs;
		variableSmoothingBufferStruct<rads> iRads;
		variableSmoothingBufferStruct<vec2> iVec2;
		variableSmoothingBufferStruct<vec3> iVec3;
		variableSmoothingBufferStruct<vec4> iVec4;
		variableSmoothingBufferStruct<quat> iQuat;
		variableSmoothingBufferStruct<mat3> iMat3;
		variableSmoothingBufferStruct<mat4> iMat4;
	}

	{
		CAGE_TESTCASE("smoothing with real");

		variableSmoothingBufferStruct<real, 4> v;
		CAGE_TEST(v.last() == 0);
		v.add(10);
		v.add(20);
		v.add(40);
		v.add(50);
		CAGE_TEST(v.last() == 50);
		CAGE_TEST(v.smooth() == 30);
	}

	{
		CAGE_TESTCASE("smoothing with uint32");

		variableSmoothingBufferStruct<uint32, 4> v;
		CAGE_TEST(v.last() == 0);
		v.add(10);
		v.add(20);
		v.add(40);
		v.add(50);
		CAGE_TEST(v.last() == 50);
		CAGE_TEST(v.smooth() == 30);
	}
}
