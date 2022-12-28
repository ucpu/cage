#include "main.h"

#include <cage-core/math.h>
#include <cage-core/variableSmoothingBuffer.h>

void testVariableSmoothingBuffer()
{
	CAGE_TESTCASE("VariableSmoothingBuffer");

	{
		CAGE_TESTCASE("compilations");
		VariableSmoothingBuffer<uint32> iUint32;
		VariableSmoothingBuffer<Real> iReal;
		VariableSmoothingBuffer<Rads> iRads;
		VariableSmoothingBuffer<Vec2> iVec2;
		VariableSmoothingBuffer<Vec3> iVec3;
		VariableSmoothingBuffer<Vec4> iVec4;
		VariableSmoothingBuffer<Quat> iQuat;
		VariableSmoothingBuffer<Transform> iTransform;
	}

	{
		CAGE_TESTCASE("smoothing with real");
		VariableSmoothingBuffer<Real, 3> v;
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
		VariableSmoothingBuffer<Quat, 4> v;
		v.seed(Quat(Degs(50), Degs(), Degs()));
		v.add(Quat(Degs(20), Degs(), Degs()));
		v.add(Quat(Degs(10), Degs(), Degs()));
		CAGE_TEST(v.current() == Quat(Degs(10), Degs(), Degs()));
		CAGE_TEST(v.oldest() == Quat(Degs(50), Degs(), Degs()));
		v.smooth();
	}

	{
		CAGE_TESTCASE("smoothing with transform");
		VariableSmoothingBuffer<Transform, 3> v;
		v.seed(Transform());
		v.add(Transform(Vec3(10, 0, 0), Quat(Degs(), Degs(40), Degs())));
		v.add(Transform(Vec3(20, 0, 0), Quat(Degs(), Degs(50), Degs())));
		v.add(Transform(Vec3(30, 0, 0), Quat(Degs(), Degs(60), Degs())));
		CAGE_TEST(v.current() == Transform(Vec3(30, 0, 0), Quat(Degs(), Degs(60), Degs())));
		CAGE_TEST(v.oldest() == Transform(Vec3(10, 0, 0), Quat(Degs(), Degs(40), Degs())));
		v.smooth();
	}
}
