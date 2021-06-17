#ifndef guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
#define guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik

#include "core.h"

#include <vector>

namespace cage
{
	// utility struct to simplify creating Holder<PointerRange<T>>
	template<class T>
	struct PointerRangeHolder : public std::vector<typename std::remove_const_t<T>>
	{
		using CT = std::remove_const_t<T>;

		PointerRangeHolder()
		{}

		explicit PointerRangeHolder(std::vector<CT> &&other) : std::vector<CT>(std::move(other))
		{}

		template<class IT>
		explicit PointerRangeHolder(IT a, IT b) : std::vector<CT>(a, b)
		{}

		template<class U>
		explicit PointerRangeHolder(const PointerRange<U> &other) : std::vector<CT>(other.begin(), other.end())
		{}

		operator Holder<PointerRange<T>>()
		{
			struct OwnedVector
			{
				std::vector<CT> vec;
				PointerRange<T> range;
			};

			Holder<OwnedVector> h = systemMemory().createHolder<OwnedVector>();
			std::swap(*this, h->vec);
			h->range = h->vec;
			return Holder<PointerRange<T>>(&h->range, std::move(h));
		}
	};
}

#endif // guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
