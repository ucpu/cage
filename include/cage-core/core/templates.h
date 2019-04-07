namespace cage
{
	namespace templates
	{
		template<bool Cond, class T> struct enable_if {};
		template<class T> struct enable_if<true, T> { typedef T type; };
		template<uintPtr Size> struct base_unsigned_type {};
		template<> struct base_unsigned_type<1> { typedef uint8 type; };
		template<> struct base_unsigned_type<2> { typedef uint16 type; };
		template<> struct base_unsigned_type<4> { typedef uint32 type; };
		template<> struct base_unsigned_type<8> { typedef uint64 type; };
		template<class T> struct underlying_type { typedef typename base_unsigned_type<sizeof(T)>::type type; };

		template<class T> struct remove_const { typedef T type; };
		template<class T> struct remove_const<const T> { typedef T type; };

		template<class T> struct remove_reference      { typedef T type; };
		template<class T> struct remove_reference<T&>  { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };

		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type  &v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type &&v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr typename remove_reference<T>::type &&move(T &&v) noexcept { return static_cast<typename remove_reference<T>::type&&>(v); }
	}

	template<class T> struct enable_bitmask_operators { static const bool enable = false; };
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ~ (T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(~static_cast<underlying>(lhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator | (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator & (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^ (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator |= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator &= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type any(T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) != 0; }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type none(T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) == 0; }

// this macro has to be used inside namespace cage
#define GCHL_ENUM_BITS(Type) template<> struct enable_bitmask_operators<Type> { static const bool enable = true; };

}
