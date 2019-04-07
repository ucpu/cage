#ifndef guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
#define guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik

#include <vector>

namespace cage
{
	template<class T>
	struct pointerRangeHolder : public std::vector<typename templates::remove_const<T>::type>
	{
		typedef typename templates::remove_const<T>::type CT;

		pointerRangeHolder()
		{}

		explicit pointerRangeHolder(std::vector<CT> &&other) : std::vector<CT>(templates::move(other))
		{}

		template<class IT>
		explicit pointerRangeHolder(IT a, IT b) : std::vector<CT>(a, b)
		{}

		template<class U>
		explicit pointerRangeHolder(const pointerRange<U> &other) : std::vector<CT>(other.begin(), other.end())
		{}

		operator holder<pointerRange<T>>()
		{
			delegate<void(void*)> d;
			d.bind<memoryArena, &memoryArena::destroy<pointerRangeHolder<T>>>(&detail::systemArena());
			pointerRangeHolder<T> *p = detail::systemArena().createObject<pointerRangeHolder<T>>();
			p->swap(*this);
			p->pr = pointerRange<T>(*p);
			holder<pointerRange<T>> h(&p->pr, p, d);
			return h;
		}

	private:
		pointerRangeHolder(const pointerRangeHolder &) = delete;
		pointerRangeHolder &operator = (const pointerRangeHolder &) = delete;
		pointerRange<T> pr;
	};
}

#endif // guard_pointerRangeHolder_h_thgfd564h64fdrht64r6ht4r56th4u7ik
