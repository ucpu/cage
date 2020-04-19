#include "main.h"
#include <cage-core/pointerRangeHolder.h>

namespace
{
	uint32 counter = 0;

	struct Test
	{
		Test()
		{
			counter++;
		}

		Test(Test &&)
		{
			counter++;
		}

		~Test()
		{
			counter--;
		}

		Test(const Test &) = delete;
		Test &operator = (const Test &) = delete;
		Test &operator = (Test &&) = default;

		void fnc()
		{}
	};

	Holder<PointerRange<uint32>> makeRangeInts()
	{
		std::vector<uint32> numbers;
		numbers.push_back(5);
		numbers.push_back(42);
		numbers.push_back(13);
		return PointerRangeHolder<uint32>(templates::move(numbers));
	}

	Holder<PointerRange<Test>> makeRangeTests()
	{
		std::vector<Test> tests;
		CAGE_TEST(counter == 0);
		tests.emplace_back();
		tests.emplace_back();
		tests.emplace_back();
		tests.emplace_back();
		CAGE_TEST(counter == 4);
		return PointerRangeHolder<Test>(templates::move(tests));
	}

	void functionTakingMutableRange(PointerRange<uint32> r)
	{
		// do nothing
	}

	void functionTakingConstRange(PointerRange<const uint32> r)
	{
		// do nothing
	}

	constexpr auto constexprFunction()
	{
		const char text[] = "ahoj";
		return PointerRange<const char>(text, text + sizeof(text)).size();
	}
}

void testPointerRange()
{
	CAGE_TESTCASE("pointer range");

	{
		CAGE_TESTCASE("basic iteration");
		std::vector<uint32> numbers;
		numbers.push_back(5);
		numbers.push_back(42);
		numbers.push_back(13);
		PointerRange<const uint32> range = numbers;
		uint32 sum = 0;
		for (auto it : range)
			sum += it;
		CAGE_TEST(sum == 5 + 42 + 13);
		CAGE_TEST(!range.empty());
		CAGE_TEST(PointerRange<const uint32>().empty());
	}

	{
		CAGE_TESTCASE("holder with pointer range <uint32>");
		auto range = makeRangeInts();
		uint32 sum = 0;
		for (auto it : range)
			sum += it;
		CAGE_TEST(sum == 5 + 42 + 13);
	}

	{
		CAGE_TESTCASE("holder with pointer range <Test>");
		{
			CAGE_TEST(counter == 0);
			auto range = makeRangeTests();
			CAGE_TEST(counter == 4);
			for (auto &it : range)
				it.fnc();
			CAGE_TEST(counter == 4);
		}
		CAGE_TEST(counter == 0);
	}

	{
		CAGE_TESTCASE("direct dereference of a holder with pointer range");
		uint32 sum = 0;
		// do not dereference the Holder! it would deallocate before iterating
		// instead iterate over the Holder directly
		for (auto it : makeRangeInts())
			sum += it;
		CAGE_TEST(sum == 5 + 42 + 13);
	}

	{
		CAGE_TESTCASE("empty ranges");
		functionTakingConstRange({});
		functionTakingMutableRange({});
		PointerRange<uint32> empty1 = {};
		PointerRange<const uint32> empty2 = {};
		Holder<PointerRange<uint32>> empty3 = {};
		Holder<PointerRange<const uint32>> empty4 = {};
	}

	{
		CAGE_TESTCASE("implicit const casts");
		Holder<PointerRange<uint32>> range = makeRangeInts();
		PointerRange<uint32> rng1 = range;
		PointerRange<const uint32> rng2 = rng1;
		//PointerRange<uint32> rng3 = rng2; // must not compile - it would drop the const
		functionTakingConstRange(rng1);
		functionTakingMutableRange(rng1);
		functionTakingConstRange(rng2);
		//functionTakingMutableRange(rng2); // must not compile - it would drop the const
		functionTakingConstRange(range);
		functionTakingMutableRange(range);
		functionTakingConstRange(makeRangeInts());
		functionTakingMutableRange(makeRangeInts());
		Holder<PointerRange<const uint32>> crange = templates::move(range);
		CAGE_TEST(crange->size() == makeRangeInts()->size());
	}

	{
		CAGE_TESTCASE("operator []");
		Holder<PointerRange<uint32>> range = makeRangeInts();
		CAGE_TEST(range[0] == 5);
		CAGE_TEST(range[1] == 42);
		CAGE_TEST(range[2] == 13);
		CAGE_TEST_ASSERTED(range[3]);
		PointerRange<uint32> rng1 = range;
		CAGE_TEST(rng1[0] == 5);
		CAGE_TEST(rng1[1] == 42);
		CAGE_TEST(rng1[2] == 13);
		CAGE_TEST_ASSERTED(rng1[3]);
	}

	{
		CAGE_TESTCASE("constexpr pointerRange");
		constexpr auto len = constexprFunction();
		CAGE_TEST(len == 5);
	}
}
