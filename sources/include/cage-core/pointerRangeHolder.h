#ifndef guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
#define guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik

#include "core.h"

#include <vector>

namespace cage
{
	// utility struct to simplify creating Holder<PointerRange<T>>
	template<class T>
	struct PointerRangeHolder : public std::vector<typename std::remove_const<T>::type>
	{
		typedef typename std::remove_const<T>::type CT;

		PointerRangeHolder()
		{}

		explicit PointerRangeHolder(std::vector<CT> &&other) : std::vector<CT>(templates::move(other))
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

			Delegate<void(void *)> d;
			d.bind<MemoryArena, &MemoryArena::destroy<OwnedVector>>(&detail::systemArena());
			OwnedVector *p = detail::systemArena().createObject<OwnedVector>();
			std::swap(*this, p->vec);
			p->range = p->vec;
			Holder<PointerRange<T>> h(&p->range, p, d);
			return h;
		}
	};
}

#endif // guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
