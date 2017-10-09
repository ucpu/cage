namespace cage
{
	namespace templates
	{
		template<class T> struct pointerRange
		{
			T *const begin;
			T *const end;
			pointerRange(T *begin, T *end) : begin(begin), end(end)
			{}
		};

		template<uint64 B, uint64 E> struct pow
		{
			static const uint64 result = B * pow<B, E - 1>::result;
		};

		template<uint64 B> struct pow<B, 0>
		{
			static const uint64 result = 1;
		};

		template<class T> struct allocatorSizeList
		{
			void *a;
			void *b;
			T t;
		};

		template<class K, class V> struct allocatorSizeMap
		{
#ifdef CAGE_SYSTEM_WINDOWS
			void *a;
			void *b;
			void *c;
			char d;
			char e;
			struct
			{
				K k;
				V v;
			} pair;
#else
			char d;
			void *a;
			void *b;
			void *c;
			struct
			{
				K k;
				V v;
			} pair;
#endif
		};

		template<class T> struct allocatorSizeSet
		{
#ifdef CAGE_SYSTEM_WINDOWS
			void *a;
			void *b;
			void *c;
			char d;
			char e;
			T t;
#else
			char d;
			void *a;
			void *b;
			void *c;
			T t;
#endif
		};

		template<bool Cond, class T = void> struct enable_if {};
		template<class T> struct enable_if<true, T> { typedef T type; };
		template<uintPtr Size> struct base_unsigned_type { };
		template<> struct base_unsigned_type<1> { typedef uint8 type; };
		template<> struct base_unsigned_type<2> { typedef uint16 type; };
		template<> struct base_unsigned_type<4> { typedef uint32 type; };
		template<> struct base_unsigned_type<8> { typedef uint64 type; };
		template<class T> struct underlying_type { typedef typename base_unsigned_type<sizeof(T)>::type type; };

		template<class T> struct remove_reference      { typedef T type; };
		template<class T> struct remove_reference<T&>  { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };

		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type  &v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type &&v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr typename remove_reference<T>::type &&move(T &&v) noexcept { return static_cast<typename remove_reference<T>::type&&>(v); }
	}

	template<class T> struct enable_bitmask_operators { static const bool enable = false; };
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ~ (T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(~static_cast<underlying>(lhs)); }
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator | (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator & (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^ (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator |= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator &= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }

#define GCHL_ENUM_BITS(Type) template<> struct enable_bitmask_operators<Type> { static const bool enable = true; };
}
