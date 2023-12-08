#include <vector>

#include "main.h"

#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>

namespace
{
	uint32 counter = 0;

	struct Test : private Noncopyable
	{
		Test() { counter++; }

		Test(Test &&) noexcept { counter++; }

		~Test() { counter--; }

		Test &operator=(Test &&) = default;

		void fnc() { (void)data_; }

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

	Holder<PointerRange<uint32>> makeEmptyRange()
	{
		return {};
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
		CAGE_TESTCASE("empty holder");
		CAGE_TEST(makeEmptyRange().size() == 0);
		CAGE_TEST(makeEmptyRange().empty());
		CAGE_TEST(makeEmptyRange().begin() == nullptr);
		CAGE_TEST(makeEmptyRange().end() == nullptr);
		CAGE_TEST(makeEmptyRange().data() == nullptr);
		{
			uint32 sum = 0;
			for (auto it : makeEmptyRange())
				sum += it;
			CAGE_TEST(sum == 0);
		}
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
		CAGE_TESTCASE("constexpr function");
		constexpr auto len = constexprFunction();
		CAGE_TEST(len == 4);
	}

	{
		CAGE_TESTCASE("pointerRange from char array");
		const char str[] = "abc\ndef\r\nghi\n\nlast";
		PointerRange<const char> r = str;
		CAGE_TEST(r[0] == 'a');
		CAGE_TEST(r.size() == 18);
	}

	{
		CAGE_TESTCASE("pointerRange from int array");
		const int arr[] = { 13, 42, 50, 20 };
		PointerRange<const int> data = arr;
		CAGE_TEST(data[0] == 13);
		CAGE_TEST(data.size() == 4);
	}

	{
		CAGE_TESTCASE("constexpr pointerRange from char array");
		static constexpr const char str[] = "hello world";
		constexpr PointerRange<const char> r = str;
		CAGE_TEST(r[0] == 'h');
		CAGE_TEST(r.size() == 11);
	}

	{
		CAGE_TESTCASE("constexpr pointerRange from int array");
		static constexpr const int arr[] = { 13, 42, 50, 20 };
		constexpr PointerRange<const int> data = arr;
		CAGE_TEST(data[0] == 13);
		CAGE_TEST(data.size() == 4);
	}

	{
		CAGE_TESTCASE("constexpr pointerRange from string literal");
		// todo fix constexpr here causes "data.size() == 18" to fail on msvc
		/*constexpr*/ PointerRange<const char> data = "abc\ndef\r\nghi\n\nlast";
		CAGE_TEST(data[0] == 'a');
		CAGE_TEST(data.size() == 18);
	}

	{
		CAGE_TESTCASE("cast method");
		uint32 arr[5] = { 13, 42 };
		PointerRange<uint32> r1 = arr;
		PointerRange<sint32> r2 = r1.cast<sint32>();
		CAGE_TEST(r2.size() == 5);
		CAGE_TEST(r2[0] == 13);
		CAGE_TEST(r2[1] == 42);
	}

	{
		CAGE_TESTCASE("cast method on holder");
		Holder<PointerRange<uint32>> r1 = makeRangeInts();
		Holder<PointerRange<sint32>> r2 = r1.share().cast<sint32>();
		CAGE_TEST(r1.size() == 3);
		CAGE_TEST(r2.size() == 3);
		CAGE_TEST(r2[0] == 5);
		CAGE_TEST(r2[1] == 42);
		CAGE_TEST(r2[2] == 13);
	}

	{
		CAGE_TESTCASE("cast to void method on holder");
		Holder<PointerRange<uint32>> r1 = makeRangeInts();
		Holder<void> r3 = r1.share().cast<void>();
		Holder<PointerRange<uint32>> r2 = r3.share().cast<PointerRange<uint32>>();
		CAGE_TEST(r1.size() == 3);
		CAGE_TEST(r2.size() == 3);
		CAGE_TEST(r2[0] == 5);
		CAGE_TEST(r2[1] == 42);
		CAGE_TEST(r2[2] == 13);
	}
}
