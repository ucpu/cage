#include "main.h"

#include <cage-core/math.h>
#include <cage-core/variableSmoothingBuffer.h>

void testVariableSmoothingBufferStruct()
{
	CAGE_TESTCASE("VariableSmoothingBuffer");

	{
		CAGE_TESTCASE("compilations");
		VariableSmoothingBuffer<uint32> iUint32;
		VariableSmoothingBuffer<real> iReal;
		VariableSmoothingBuffer<rads> iRads;
		VariableSmoothingBuffer<vec2> iVec2;
		VariableSmoothingBuffer<vec3> iVec3;
		VariableSmoothingBuffer<vec4> iVec4;
		VariableSmoothingBuffer<quat> iQuat;
	}

	{
		CAGE_TESTCASE("smoothing with real");
		VariableSmoothingBuffer<real, 3> v;
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
		VariableSmoothingBuffer<uint32, 4> v;
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
		VariableSmoothingBuffer<quat, 4> v;
		v.seed(quat(degs(50), degs(), degs()));
		v.add(quat(degs(20), degs(), degs()));
		v.add(quat(degs(10), degs(), degs()));
		CAGE_TEST(v.current() == quat(degs(10), degs(), degs()));
		CAGE_TEST(v.oldest() == quat(degs(50), degs(), degs()));
		v.smooth();
	}
}
