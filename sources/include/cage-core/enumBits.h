#ifndef guard_enumBits_h_fdtgh456fgzj4
#define guard_enumBits_h_fdtgh456fgzj4

#include <cage-core/core.h>

namespace cage
{
	template<class T>
	struct enable_bitmask_operators
	{
		static constexpr bool enable = false;
	};
	template<class T>
	constexpr bool enable_bitmask_operators_v = enable_bitmask_operators<T>::enable;

	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator~(T lhs) noexcept
	{
		return static_cast<T>(~static_cast<std::underlying_type_t<T>>(lhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator|(T lhs, T rhs) noexcept
	{
		return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator&(T lhs, T rhs) noexcept
	{
		return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator^(T lhs, T rhs) noexcept
	{
		return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator|=(T &lhs, T rhs) noexcept
	{
		return lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator&=(T &lhs, T rhs) noexcept
	{
		return lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr T operator^=(T &lhs, T rhs) noexcept
	{
		return lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs));
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr bool any(T lhs) noexcept
	{
		return static_cast<std::underlying_type_t<T>>(lhs) != 0;
	}
	template<class T>
	requires(enable_bitmask_operators_v<T>)
	CAGE_FORCE_INLINE constexpr bool none(T lhs) noexcept
	{
		return static_cast<std::underlying_type_t<T>>(lhs) == 0;
	}

	// this macro has to be used inside namespace cage
#define GCHL_ENUM_BITS(TYPE) \
	template<> \
	struct enable_bitmask_operators<TYPE> \
	{ \
		static_assert(std::is_enum_v<TYPE>); \
		static constexpr bool enable = true; \
	};
}

#endif // guard_enumBits_h_fdtgh456fgzj4
