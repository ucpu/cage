#include "main.h"

#include <cage-core/math.h>
#include <cage-core/rectPacking.h>

void testRectPacking()
{
	CAGE_TESTCASE("rect packing");

	Holder<RectPacking> rp = newRectPacking();
	rp->resize(10);
	for (uint32 i = 0; i < 10; i++)
		rp->data()[i] = PackingRect{ i, (uint32)randomRange(10, 30), (uint32)randomRange(10, 30) };
	RectPackingSolveConfig cfg;
	cfg.margin = 2;
	cfg.width = cfg.height = 30;
	CAGE_TEST(!rp->solve(cfg)); // not enough space
	cfg.width = 130;
	cfg.height = 100;
	CAGE_TEST(rp->solve(cfg)); // always enough
	CAGE_TEST(rp->data().size() == 10);
	for (const auto &it : rp->data())
		CAGE_TEST(it.x <= 120 && it.y <= 90);
}
