#ifndef guard_enumerate_h_spo4ysdsxf41pop13686sa74g
#define guard_enumerate_h_spo4ysdsxf41pop13686sa74g

#include "core.h"

namespace cage
{
	namespace privat
	{
		template<class It1, class It2, class Counter, class Range>
		struct Enumerate : private Range
		{
			template<class It>
			struct Iterator
			{
				struct Pair
				{
					It it;
					Counter index;

					CAGE_FORCE_INLINE constexpr Pair(const It &it, const Counter &index) : it(it), index(index)
					{}

					CAGE_FORCE_INLINE constexpr auto &get() const noexcept
					{
						return *it;
					}

					CAGE_FORCE_INLINE constexpr auto &operator * () const noexcept
					{
						return *it;
					}

					CAGE_FORCE_INLINE constexpr const auto &operator -> () const noexcept
					{
						return it;
					}
				};

				CAGE_FORCE_INLINE constexpr Iterator(const It &it, const Counter &index) : p(it, index)
				{}

				template<class U>
				CAGE_FORCE_INLINE constexpr bool operator == (const Iterator<U> &other) const
				{
					return p.it == other.p.it;
				}

				template<class U>
				CAGE_FORCE_INLINE constexpr bool operator != (const Iterator<U> &other) const
				{
					return p.it != other.p.it;
				}

				CAGE_FORCE_INLINE constexpr Iterator operator ++ ()
				{
					auto r = *this;
					p.it++;
					p.index++;
					return r;
				}

				CAGE_FORCE_INLINE constexpr Iterator &operator ++ (int)
				{
					++p.it;
					++p.index;
					return *this;
				}

				CAGE_FORCE_INLINE constexpr const Pair &operator * () const
				{
					return p;
				}

			private:
				Pair p;
			};

			CAGE_FORCE_INLINE constexpr auto begin() const { return Iterator<It1>(it1, start); }
			CAGE_FORCE_INLINE constexpr auto end() const { return Iterator<It2>(it2, start); }

			CAGE_FORCE_INLINE constexpr Enumerate(const It1 &it1, const It2 &it2, const Counter &start, Range &&range) : Range(std::move(range)), it1(it1), it2(it2), start(start)
			{}

		private:
			const It1 it1;
			const It2 it2;
			const Counter start;
		};

		struct EnumerateNone
		{};

		// generic
		template<class It1, class It2, class Counter, class Range = EnumerateNone>
		CAGE_FORCE_INLINE constexpr auto enumerate(const It1 &it1, const It2 &it2, const Counter &start, Range &&range = EnumerateNone())
		{
			return Enumerate<It1, It2, Counter, Range>(it1, it2, start, std::move(range));
		}
	}

	// two iterators
	template<class It1, class It2, class Counter = uintPtr>
	CAGE_FORCE_INLINE constexpr auto enumerate(It1 it1, It2 it2, Counter start = Counter())
	{
		return privat::enumerate(it1, it2, start);
	}

	// l-value range
	template<class Range, class Counter = typename Range::size_type>
	CAGE_FORCE_INLINE constexpr auto enumerate(Range &range)
	{
		return privat::enumerate(range.begin(), range.end(), Counter());
	}

	// const range
	template<class Range, class Counter = typename Range::size_type>
	CAGE_FORCE_INLINE constexpr auto enumerate(const Range &range)
	{
		return privat::enumerate(range.begin(), range.end(), Counter());
	}

	// r-value range
	template<class Range, class Counter = typename Range::size_type>
	CAGE_FORCE_INLINE constexpr auto enumerate(Range &&range)
	{
		auto b = range.begin();
		auto e = range.end();
		// we need to extend lifetime of the temporary (we take ownership of the variable)
		return privat::enumerate(b, e, Counter(), std::move(range));
	}

	// c array
	template<class T, uintPtr N, class Counter = uintPtr>
	CAGE_FORCE_INLINE constexpr auto enumerate(T (&range)[N])
	{
		return privat::enumerate(range + 0, range + N, Counter());
	}
}

#endif // guard_enumerate_h_spo4ysdsxf41pop13686sa74g
