#ifndef guard_source_location_dummy_dfrg4sd5g
#define guard_source_location_dummy_dfrg4sd5g

namespace std
{
	struct source_location
	{
		static consteval source_location current() noexcept { return {}; }
		constexpr source_location() noexcept {}
		constexpr uint_least32_t line() const noexcept { return 0; }
		constexpr uint_least32_t column() const noexcept { return 0; }
		constexpr const char *file_name() const noexcept { return ""; }
		constexpr const char *function_name() const noexcept { return ""; }
	};
}

#endif // guard_source_location_dummy_dfrg4sd5g
