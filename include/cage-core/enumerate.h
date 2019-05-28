#ifndef guard_enumerate_h_spo4ysdsxf41pop13686sa74g
#define guard_enumerate_h_spo4ysdsxf41pop13686sa74g

namespace cage
{
	namespace privat
	{
		template<class Range, class It1, class It2, class Counter>
		struct enumerateStruct : private Range
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
				bool operator == (const iterator<U> &other) const
				{
					return p.it == other.p.it;
				}

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

			enumerateStruct(Range &&range, It1 it1, It2 it2, Counter start) : Range(templates::move(range)), it1(it1), it2(it2), start(start)
			{}

		private:
			const It1 it1;
			const It2 it2;
			const Counter start;
		};

		// generic
		template<class Range, class It1, class It2, class Counter>
		inline auto enumerate(Range &&range, It1 it1, It2 it2, Counter start)
		{
			return privat::enumerateStruct<Range, It1, It2, Counter>(templates::move(range), it1, it2, start);
		}
	}

	// two iterators
	template<class It1, class It2, class Counter = uintPtr>
	inline auto enumerate(It1 it1, It2 it2, Counter start = Counter())
	{
		struct none {};
		return privat::enumerate(none(), it1, it2, start);
	}

	// l-value range
	template<class Range, class Counter = typename Range::size_type>
	inline auto enumerate(Range &range)
	{
		struct none {};
		return privat::enumerate(none(), range.begin(), range.end(), Counter());
	}

	// const range
	template<class Range, class Counter = typename Range::size_type>
	inline auto enumerate(const Range &range)
	{
		struct none {};
		return privat::enumerate(none(), range.begin(), range.end(), Counter());
	}

	// r-value range
	template<class Range, class Counter = typename Range::size_type>
	inline auto enumerate(Range &&range)
	{
		// we need to extend lifetime of the temporary (we take ownership of the variable)
		return privat::enumerate(templates::move(range), range.begin(), range.end(), Counter());
	}

	// c array
	template<class T, uintPtr N, class Counter = uintPtr>
	inline auto enumerate(T (&range)[N])
	{
		struct none {};
		return privat::enumerate(none(), range, range + N, Counter());
	}
}

#endif // guard_enumerate_h_spo4ysdsxf41pop13686sa74g
