#include "main.h"
#include <cage-core/typeRegistry.h>
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

void testTypeRegistry()
{
	CAGE_TESTCASE("typeRegistry");

	using namespace detail;

	const uint32 val = typeRegistryId<uint16>();
	CAGE_TEST(val == typeRegistryId<uint16>()); // multiple calls to typeRegistryId are allowed
	CAGE_TEST(val != typeRegistryId<uint64>());
	CAGE_TEST(typeRegistryId<float>() != typeRegistryId<double>());
	CAGE_TEST(typeRegistryId<uint16>() != typeRegistryId<uintPtr>());
	CAGE_TEST(typeRegistryId<void*>() != typeRegistryId<uintPtr>());
	CAGE_TEST(typeRegistryId<uint32>() != typeRegistryId<uint64>());
#ifdef CAGE_ARCHITECTURE_64
	CAGE_TEST(typeRegistryId<uint64>() == typeRegistryId<uintPtr>());
#else
	CAGE_TEST(typeRegistryId<uint32>() == typeRegistryId<uintPtr>());
#endif // CAGE_ARCHITECTURE_64

	{
		CAGE_TESTCASE("performance");
		measurePerformance<&typeRegistryId<uint64>>("actual implementation");
		measurePerformance<&dummyRegistryId>("dummy implementation");
	}
}
