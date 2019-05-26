#ifndef guard_enumerate_h_spo4ysdsxf41pop13686sa74g
#define guard_enumerate_h_spo4ysdsxf41pop13686sa74g

namespace cage
{
	namespace privat
	{
		template<class Container, class It1, class It2, class Counter>
		struct enumerateStruct
		{
			template<class It>
			struct iterator
			{
				struct pair
				{
					It it;
					Counter cnt;

					pair(It it, Counter cnt) : it(it), cnt(cnt)
					{}

					auto operator * () const
					{
						return *it;
					}

					auto operator -> () const
					{
						return it;
					}
				};

				pair p;

				iterator(It it, Counter cnt) : p(it, cnt)
				{}

				template<class U>
				bool operator != (const iterator<U> &other) const
				{
					return p.it != other.p.it;
				}

				iterator operator ++ ()
				{
					auto r(*this);
					p.it++;
					p.cnt++;
					return r;
				}

				iterator &operator ++ (int)
				{
					++p.it;
					++p.cnt;
					return *this;
				}

				const pair &operator * () const
				{
					return p;
				}
			};

			auto begin() { return iterator<It1>(it1, start); }
			auto end() { return iterator<It2>(it2, start); }

			enumerateStruct(Container &&cont, It1 it1, It2 it2, Counter start) : cont(templates::move(cont)), it1(it1), it2(it2), start(start)
			{}

		private:
			const Container cont; // extend lifetime of the container
			const It1 it1;
			const It2 it2;
			const Counter start;
		};
	}

	template<class It1, class It2, class Counter = uintPtr>
	inline auto enumerate(It1 it1, It2 it2, Counter start = 0)
	{
		struct none {};
		return privat::enumerateStruct<none, It1, It2, Counter>(none(), it1, it2, start);
	}

	template<class Container, class Counter = typename Container::size_type>
	inline auto enumerate(Container &cont)
	{
		struct none {};
		return privat::enumerateStruct<none, decltype(cont.begin()), decltype(cont.end()), Counter>(none(), cont.begin(), cont.end(), 0);
	}

	template<class Container, class Counter = typename Container::size_type>
	inline auto enumerate(const Container &cont)
	{
		struct none {};
		return privat::enumerateStruct<none, decltype(cont.begin()), decltype(cont.end()), Counter>(none(), cont.begin(), cont.end(), 0);
	}

	template<class Container, class Counter = typename Container::size_type>
	inline auto enumerate(Container &&cont)
	{
		return privat::enumerateStruct<Container, decltype(cont.begin()), decltype(cont.end()), Counter>(templates::move(cont), cont.begin(), cont.end(), 0);
	}
}

#endif // guard_enumerate_h_spo4ysdsxf41pop13686sa74g
