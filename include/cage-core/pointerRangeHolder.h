#ifndef guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
#define guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik

#include "core.h"

#include <vector>

namespace cage
{
	template<class T>
	struct PointerRangeHolder : public std::vector<typename templates::remove_const<T>::type>
	{
		typedef typename templates::remove_const<T>::type CT;

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
			Delegate<void(void*)> d;
			d.bind<MemoryArena, &MemoryArena::destroy<PointerRangeHolder<T>>>(&detail::systemArena());
			PointerRangeHolder<T> *p = detail::systemArena().createObject<PointerRangeHolder<T>>();
			p->swap(*this);
			p->pr = PointerRange<T>(*p);
			Holder<PointerRange<T>> h(&p->pr, p, d);
			return h;
		}

	private:
		PointerRangeHolder(const PointerRangeHolder &) = delete;
		PointerRangeHolder &operator = (const PointerRangeHolder &) = delete;
		PointerRange<T> pr;
	};
}

#endif // guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
