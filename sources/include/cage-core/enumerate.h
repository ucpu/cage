#ifndef guard_enumerate_h_spo4ysdsxf41pop13686sa74g
#define guard_enumerate_h_spo4ysdsxf41pop13686sa74g

#include "core.h"

namespace cage
{
	namespace privat
	{
		template<class Range, class It1, class It2, class Counter>
		struct Enumerate : private Range
		{
			template<class It>
			struct Iterator
			{
				struct Pair
				{
					It it;
					Counter index;

					constexpr Pair(const It &it, const Counter &index) : it(it), index(index)
					{}

					constexpr auto &get() const noexcept
					{
						return *it;
					}

					constexpr auto &operator * () const noexcept
					{
						return *it;
					}

					constexpr const auto &operator -> () const noexcept
					{
						return it;
					}
				};

				constexpr Iterator(const It &it, const Counter &index) : p(it, index)
				{}

				template<class U>
				constexpr bool operator == (const Iterator<U> &other) const
				{
					return p.it == other.p.it;
				}

				template<class U>
				constexpr bool operator != (const Iterator<U> &other) const
				{
					return p.it != other.p.it;
				}

				constexpr Iterator operator ++ ()
				{
					auto r(*this);
					p.it++;
					p.index++;
					return r;
				}

				constexpr Iterator &operator ++ (int)
				{
					++p.it;
					++p.index;
					return *this;
				}

				constexpr const Pair &operator * () const
				{
					return p;
				}

			private:
				Pair p;
			};

			constexpr auto begin() const { return Iterator<It1>(it1, start); }
			constexpr auto end() const { return Iterator<It2>(it2, start); }

			constexpr Enumerate(Range &&range, const It1 &it1, const It2 &it2, const Counter &start) : Range(std::move(range)), it1(it1), it2(it2), start(start)
			{}

		private:
			const It1 it1;
			const It2 it2;
			const Counter start;
		};

		// generic
		template<class Range, class It1, class It2, class Counter>
		inline constexpr auto enumerate(Range &&range, const It1 &it1, const It2 &it2, const Counter &start)
		{
			return Enumerate<Range, It1, It2, Counter>(std::move(range), it1, it2, start);
		}

		struct EnumerateNone
		{};
	}

	// two iterators
	template<class It1, class It2, class Counter = uintPtr>
	inline constexpr auto enumerate(It1 it1, It2 it2, Counter start = Counter())
	{
		return privat::enumerate(privat::EnumerateNone(), it1, it2, start);
	}

	// l-value range
	template<class Range, class Counter = typename Range::size_type>
	inline constexpr auto enumerate(Range &range)
	{
		return privat::enumerate(privat::EnumerateNone(), range.begin(), range.end(), Counter());
	}

	// const range
	template<class Range, class Counter = typename Range::size_type>
	inline constexpr auto enumerate(const Range &range)
	{
		return privat::enumerate(privat::EnumerateNone(), range.begin(), range.end(), Counter());
	}

	// r-value range
	template<class Range, class Counter = typename Range::size_type>
	inline constexpr auto enumerate(Range &&range)
	{
		// we need to extend lifetime of the temporary (we take ownership of the variable)
		return privat::enumerate(std::move(range), range.begin(), range.end(), Counter());
	}

	// c array
	template<class T, uintPtr N, class Counter = uintPtr>
	inline constexpr auto enumerate(T (&range)[N])
	{
		return privat::enumerate(privat::EnumerateNone(), range + 0, range + N, Counter());
	}
}

#endif // guard_enumerate_h_spo4ysdsxf41pop13686sa74g
