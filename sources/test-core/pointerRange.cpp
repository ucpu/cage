#include "main.h"
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>

#include <vector>

namespace
{
	uint32 counter = 0;

	struct Test
	{
		Test()
		{
			counter++;
		}

		Test(Test &&) noexcept
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

	private:
		uint32 data_[5] = {};
	};

	Holder<PointerRange<uint32>> makeRangeInts()
	{
		std::vector<uint32> numbers;
		numbers.push_back(5);
		numbers.push_back(42);
		numbers.push_back(13);
		return PointerRangeHolder<uint32>(std::move(numbers));
	}

	Holder<PointerRange<Test>> makeRangeTests()
	{
		const uint32 init = counter;
		std::vector<Test> tests;
		CAGE_TEST(counter == init);
		tests.emplace_back();
		tests.emplace_back();
		tests.emplace_back();
		tests.emplace_back();
		CAGE_TEST(counter == init + 4);
		return PointerRangeHolder<Test>(std::move(tests));
	}

	void functionTakingMutableRange(PointerRange<Test> r)
	{
		// do nothing
	}

	void functionTakingConstRange(PointerRange<const Test> r)
	{
		// do nothing
	}

	void functionTakingMutableRangeReference(PointerRange<Test> &r, uintPtr newSize)
	{
		r = { r.begin(), r.begin() + newSize };
	}

	void functionTakingConstRangeReference(PointerRange<const Test> &r, uintPtr newSize)
	{
		r = { r.begin(), r.begin() + newSize };
	}

	constexpr auto constexprFunction()
	{
		const char text[] = "ahoj";
		return PointerRange<const char>(text).size();
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
		{
			PointerRange<uint32> empty1;
			CAGE_TEST(empty1.empty());
			PointerRange<const uint32> empty2;
			CAGE_TEST(empty2.empty());
			Holder<PointerRange<uint32>> empty3;
			CAGE_TEST(empty3.empty());
			Holder<PointerRange<const uint32>> empty4;
			CAGE_TEST(empty4.empty());
		}
		{
			PointerRange<uint32> empty1 = {};
			CAGE_TEST(empty1.empty());
			PointerRange<const uint32> empty2 = {};
			CAGE_TEST(empty2.empty());
			Holder<PointerRange<uint32>> empty3 = {};
			CAGE_TEST(empty3.empty());
			Holder<PointerRange<const uint32>> empty4 = {};
			CAGE_TEST(empty4.empty());
		}
	}

	{
		CAGE_TESTCASE("implicit const casts");
		{
			Holder<PointerRange<Test>> range = makeRangeTests();
			PointerRange<Test> rng1 = range;
			PointerRange<const Test> rng2 = rng1;
			//PointerRange<Test> rng3 = rng2; // must not compile - it would drop the const
			functionTakingConstRange(rng1);
			functionTakingMutableRange(rng1);
			functionTakingConstRange(rng2);
			//functionTakingMutableRange(rng2); // must not compile - it would drop the const
			functionTakingConstRange(range);
			functionTakingMutableRange(range);
			functionTakingConstRange(makeRangeTests());
			functionTakingMutableRange(makeRangeTests());
			Holder<PointerRange<const Test>> crange = std::move(range);
			CAGE_TEST(crange->size() == 4);
			CAGE_TEST(counter == 4);
		}
		CAGE_TEST(counter == 0);
	}

	{
		CAGE_TESTCASE("views");
		{
			Test testsArray[5];
			CAGE_TEST(counter == 5);
			PointerRange<Test> mutView = testsArray;
			PointerRange<const Test> constView = mutView;
			CAGE_TEST(counter == 5);
			CAGE_TEST(mutView.size() == 5);
			CAGE_TEST(constView.size() == 5);
			functionTakingMutableRange(mutView);
			//functionTakingMutableRange(constView); // must not compile - it would drop the const
			functionTakingConstRange(mutView);
			functionTakingConstRange(constView);
			CAGE_TEST(mutView.size() == 5);
			CAGE_TEST(constView.size() == 5);
			functionTakingMutableRangeReference(mutView, 1);
			CAGE_TEST(mutView.size() == 1);
			//functionTakingMutableRangeReference(constView, 2); // must not compile - it would drop the const
			//functionTakingConstRangeReference(mutView, 3); // must not compile - it would drop the const
			functionTakingConstRangeReference(constView, 4);
			CAGE_TEST(constView.size() == 4);
			CAGE_TEST(counter == 5);
		}
		CAGE_TEST(counter == 0);
	}

	{
		CAGE_TESTCASE("bufferCast");
		struct Item
		{
			char dummy[42] = {};
		};
		Item testsArray[5];
		PointerRange<Item> mutView = testsArray;
		PointerRange<const char> charView = bufferCast(mutView);
		PointerRange<const char> charViewExplicit = bufferCast<const char>(mutView);
		CAGE_TEST(charView.size() == 5 * sizeof(Item));
		PointerRange<const Item> constView = bufferCast<const Item>(charView);
		CAGE_TEST(constView.size() == 5);
	}

	{
		CAGE_TESTCASE("vector");
		struct Item
		{
			char dummy[42] = {};
		};
		std::vector<Item> vec;
		vec.resize(13);
		PointerRange<Item> mutView = vec;
		CAGE_TEST(mutView.size() == 13);
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
		CAGE_TESTCASE("move Holder<PointerRange<T>>");
		Holder<PointerRange<uint32>> r1 = makeRangeInts();
		Holder<PointerRange<uint32>> r2 = std::move(r1);
		CAGE_TEST(r1.empty());
		CAGE_TEST(r2.size() == 3);
	}

	{
		CAGE_TESTCASE("move Holder<PointerRange<const T>>");
		Holder<PointerRange<const uint32>> r1 = makeRangeInts();
		Holder<PointerRange<const uint32>> r2 = std::move(r1);
		CAGE_TEST(r1.empty());
		CAGE_TEST(r2.size() == 3);
	}

	{
		CAGE_TESTCASE("move PointerRangeHolder<T>");
		PointerRangeHolder<uint32> r1;
		r1.emplace_back(13);
		r1.emplace_back(42);
		PointerRangeHolder<uint32> r2 = std::move(r1);
		CAGE_TEST(r1.empty());
		CAGE_TEST(r2.size() == 2);
	}

	{
		CAGE_TESTCASE("assign to Holder<PointerRange<const T>>");
		Holder<PointerRange<const uint32>> r1 = makeRangeInts();
		r1 = makeRangeInts();
		CAGE_TEST(r1.size() == 3);
	}

	{
		CAGE_TESTCASE("constexpr pointerRange");
		constexpr auto len = constexprFunction();
		CAGE_TEST(len == 4);
	}

	{
		CAGE_TESTCASE("pointerRange from char array");
		constexpr const char str[] = "hello world";
		PointerRange<const char> r = str;
	}
}
