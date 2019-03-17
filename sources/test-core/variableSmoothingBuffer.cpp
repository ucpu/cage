#include "main.h"

#include <cage-core/math.h>
#include <cage-core/variableSmoothingBuffer.h>

void testVariableSmoothingBufferStruct()
{
	CAGE_TESTCASE("variableSmoothingBuffer");

	{
		CAGE_TESTCASE("compilations");
		variableSmoothingBufferStruct<uint32> iUint32;
		variableSmoothingBufferStruct<real> iReal;
		variableSmoothingBufferStruct<rads> iRads;
		variableSmoothingBufferStruct<vec2> iVec2;
		variableSmoothingBufferStruct<vec3> iVec3;
		variableSmoothingBufferStruct<vec4> iVec4;
		variableSmoothingBufferStruct<quat> iQuat;
	}

	{
		CAGE_TESTCASE("smoothing with real");
		variableSmoothingBufferStruct<real, 3> v;
		CAGE_TEST(v.current() == 0);
		v.seed(13);
		v.add(90);
		v.add(30);
		v.add(50);
		v.add(10);
		CAGE_TEST(v.current() == 10);
		CAGE_TEST(v.oldest() == 30);
		CAGE_TEST(v.smooth() == 30);
		CAGE_TEST(v.max() == 50);
		CAGE_TEST(v.min() == 10);
	}

	{
		CAGE_TESTCASE("smoothing with uint32");
		variableSmoothingBufferStruct<uint32, 4> v;
		CAGE_TEST(v.current() == 0);
		v.add(20);
		v.add(10);
		v.add(50);
		v.add(40);
		CAGE_TEST(v.current() == 40);
		CAGE_TEST(v.oldest() == 20);
		CAGE_TEST(v.smooth() == 30);
		CAGE_TEST(v.max() == 50);
		CAGE_TEST(v.min() == 10);
	}

	{
		CAGE_TESTCASE("smoothing with quat");
		variableSmoothingBufferStruct<quat, 4> v;
		v.seed(quat(degs(50), degs(), degs()));
		v.add(quat(degs(20), degs(), degs()));
		v.add(quat(degs(10), degs(), degs()));
		CAGE_TEST(v.current() == quat(degs(10), degs(), degs()));
		CAGE_TEST(v.oldest() == quat(degs(50), degs(), degs()));
		v.smooth();
	}
}
