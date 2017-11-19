namespace cage
{
	namespace privat
	{
		template<bool ToSig, bool FromSig>
		struct numeric_cast_helper_signed
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT_RUNTIME(from >= detail::numeric_limits<To>::min(), "numeric cast failure", from, detail::numeric_limits<To>::min());
				CAGE_ASSERT_RUNTIME(from <= detail::numeric_limits<To>::max(), "numeric cast failure", from, detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<false, true> // signed -> unsigned
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT_RUNTIME(from >= 0, "numeric cast failure", from);
				typedef typename detail::numeric_limits<From>::make_unsigned unsgFrom;
				CAGE_ASSERT_RUNTIME(static_cast<unsgFrom>(from) <= detail::numeric_limits<To>::max(), "numeric cast failure", from, static_cast<unsgFrom>(from), detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<true, false> // unsigned -> signed
		{
			template<class To, class From>
			static To cast(From from)
			{
				typedef typename detail::numeric_limits<To>::make_unsigned unsgTo;
				CAGE_ASSERT_RUNTIME(from <= static_cast<unsgTo>(detail::numeric_limits<To>::max()), "numeric cast failure", from, detail::numeric_limits<To>::max(), static_cast<unsgTo>(detail::numeric_limits<To>::max()));
				return static_cast<To>(from);
			}
		};

		template<bool Specialized>
		struct numeric_cast_helper_specialized {};

		template<>
		struct numeric_cast_helper_specialized <true>
		{
			template<class To, class From> static To cast(From from)
			{
				return numeric_cast_helper_signed<detail::numeric_limits<To>::is_signed, detail::numeric_limits<From>::is_signed>::template cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_specialized <false>
		{
			template<class To, class From> static To cast(From from)
			{
				CAGE_ASSERT_COMPILE(false, numeric_cast_is_only_allowed_for_numbers);
				return 0;
			}
		};
	}

	template<class To, class From>
	To numeric_cast(From from)
	{
		return privat::numeric_cast_helper_specialized<detail::numeric_limits<To>::is_specialized && detail::numeric_limits<From>::is_specialized>::template cast<To>(from);
	};

	template<class To, class From>
	To numeric_cast(From *from)
	{
		return numeric_cast<To>((uintPtr)from);
	};
}
