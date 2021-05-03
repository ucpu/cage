#include "main.h"
#include <cage-core/typeIndex.h>
#include <cage-core/timer.h>

namespace
{
	CAGE_FORCE_INLINE uint32 dummyRegistryId()
	{
		return 42;
	}

	template<uint32(*FNC)()>
	void measurePerformance(const char *name)
	{
		Holder<Timer> timer = newTimer();
		volatile uint32 tmp = 0;
		for (uint32 i = 0; i < 1000000; i++)
			tmp = FNC();
		const uint64 duration = timer->microsSinceStart();
		CAGE_TEST(tmp != 0);
		CAGE_LOG(SeverityEnum::Info, "performance", stringizer() + name + ": " + duration);
	}
}

void testTypeIndex()
{
	CAGE_TESTCASE("typeIndex");

	using namespace detail;

	const uint32 val = typeIndex<uint16>();
	CAGE_TEST(val == typeIndex<uint16>()); // multiple calls to typeIndex are allowed
	CAGE_TEST(val != typeIndex<uint64>());
	CAGE_TEST(typeIndex<float>() != typeIndex<double>());
	CAGE_TEST(typeIndex<uint16>() != typeIndex<uintPtr>());
	CAGE_TEST(typeIndex<void*>() != typeIndex<uintPtr>());
	CAGE_TEST(typeIndex<uint32>() != typeIndex<uint64>());
#ifdef CAGE_ARCHITECTURE_64
	CAGE_TEST(typeIndex<uint64>() == typeIndex<uintPtr>());
#else
	CAGE_TEST(typeIndex<uint32>() == typeIndex<uintPtr>());
#endif // CAGE_ARCHITECTURE_64

	{
		CAGE_TESTCASE("performance");
		measurePerformance<&typeIndex<uint64>>("actual implementation");
		measurePerformance<&dummyRegistryId>("dummy implementation");
	}
}
