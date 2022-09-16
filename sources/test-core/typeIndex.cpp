#include "main.h"

#include <cage-core/typeIndex.h>
#include <cage-core/timer.h>

namespace
{
	template<uint32(*FNC)()>
	void measurePerformance(const char *name)
	{
		Holder<Timer> timer = newTimer();
		volatile uint32 tmp = 0;
		for (uint32 i = 0; i < 1000000; i++)
			tmp = FNC();
		const uint64 duration = timer->duration();
		CAGE_TEST(tmp != 0);
		CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + name + ": " + duration);
	}

	template<class T>
	uintPtr typeSize()
	{
		return detail::typeSizeByIndex(detail::typeIndex<T>());
	}

	template<class T>
	uintPtr typeAlignment()
	{
		return detail::typeAlignmentByIndex(detail::typeIndex<T>());
	}

	template<class T>
	uint32 dummyRegistryId()
	{
		return 42;
	}

	template<class T>
	uint32 typeIndexWrapper()
	{
		return detail::typeIndex<T>();
	}

	template<class T>
	uint32 typeHashWrapper()
	{
		return detail::typeHash<T>();
	}
}

void testTypeIndex()
{
	CAGE_TESTCASE("typeIndex");

	using namespace detail;

	{
		CAGE_TESTCASE("typeIndex");
		const uint32 val = typeIndex<uint16>();
		CAGE_TEST(val == typeIndex<uint16>()); // multiple calls to typeIndex are allowed
		CAGE_TEST(val != typeIndex<uint64>());
		CAGE_TEST(typeIndex<float>() != typeIndex<double>());
		CAGE_TEST(typeIndex<uint16>() != typeIndex<uintPtr>());
		CAGE_TEST(typeIndex<void *>() != typeIndex<uintPtr>());
		CAGE_TEST(typeIndex<uint32>() != typeIndex<uint64>());
#ifdef CAGE_ARCHITECTURE_64
		CAGE_TEST(typeIndex<uint64>() == typeIndex<uintPtr>());
#else
		CAGE_TEST(typeIndex<uint32>() == typeIndex<uintPtr>());
#endif // CAGE_ARCHITECTURE_64
	}

	{
		CAGE_TESTCASE("typeHash");
		const uint32 val = typeHash<uint16>();
		CAGE_TEST(val == typeHash<uint16>()); // multiple calls to typeIndex are allowed
		CAGE_TEST(val != typeHash<uint64>());
		CAGE_TEST(typeHash<float>() != typeHash<double>());
		CAGE_TEST(typeHash<uint16>() != typeHash<uintPtr>());
		CAGE_TEST(typeHash<void *>() != typeHash<uintPtr>());
		CAGE_TEST(typeHash<uint32>() != typeHash<uint64>());
	}

	{
		CAGE_TESTCASE("typeHashByIndex");
		CAGE_TEST(typeHash<float>() == typeHashByIndex(typeIndex<float>()));
		CAGE_TEST(typeHash<double>() == typeHashByIndex(typeIndex<double>()));
		CAGE_TEST(typeHash<float>() != typeHashByIndex(typeIndex<double>()));
	}

	{
		CAGE_TESTCASE("typeHashByIndex");
		CAGE_TEST(typeIndex<float>() == typeIndexByHash(typeHash<float>()));
		CAGE_TEST(typeIndex<double>() == typeIndexByHash(typeHash<double>()));
		CAGE_TEST(typeIndex<float>() != typeIndexByHash(typeHash<double>()));
	}

	{
		CAGE_TESTCASE("size and alignment of types");
		const uint32 val = typeIndex<uint16>();
		CAGE_TEST(typeSizeByIndex(val) == sizeof(uint16));
		CAGE_TEST(typeAlignmentByIndex(val) == alignof(uint16));
		CAGE_TEST(typeSize<String>() == sizeof(String));
		CAGE_TEST(typeAlignment<String>() == alignof(String));
		CAGE_TEST(typeSize<uintPtr>() == sizeof(uintPtr));
		CAGE_TEST(typeAlignment<uintPtr>() == alignof(uintPtr));
		CAGE_TEST(typeSize<char>() == sizeof(char));
		CAGE_TEST(typeAlignment<char>() == alignof(char));
		CAGE_TEST(typeSize<void>() == 0); // special case - sizeof(void) is not allowed
		CAGE_TEST(typeAlignment<void>() == 0);
	}

	{
		CAGE_TESTCASE("performance");
		measurePerformance<&dummyRegistryId<uint64>>("dummy implementation");
		measurePerformance<&typeIndexWrapper<uint64>>("typeIndex");
		measurePerformance<&typeHashWrapper<uint64>>("typeHash");
	}
}
