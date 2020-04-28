#include "main.h"
#include <cage-core/math.h>
#include <cage-core/rectPacking.h>

void testRectPacking()
{
	CAGE_TESTCASE("rect packing");

	RectPackingCreateConfig packingCfg;
	packingCfg.margin = 2;
	Holder<RectPacking> rp = newRectPacking(packingCfg);
	for (uint32 i = 0; i < 10; i++)
		rp->add(i, randomRange(10, 30), randomRange(10, 30));
	CAGE_TEST(!rp->solve(30, 30)); // not enough space
	CAGE_TEST(rp->solve(130, 100)); // always enough
	CAGE_TEST(rp->count() == 10);
	for (uint32 i = 0; i < rp->count(); i++)
	{
		uint32 id, x, y;
		rp->get(i, id, x, y);
		CAGE_TEST(x <= 120 && y <= 90);
	}
}
