namespace cage
{
	typedef unsigned char uint8;
	typedef signed char sint8;
	typedef unsigned short uint16;
	typedef signed short sint16;
	typedef unsigned int uint32;
	typedef signed int sint32;
#ifdef CAGE_SYSTEM_WINDOWS
	typedef unsigned long long uint64;
	typedef signed long long sint64;
#else
	typedef unsigned long uint64;
	typedef signed long sint64;
#endif
#ifdef CAGE_ARCHITECTURE_64
	typedef uint64 uintPtr;
	typedef sint64 sintPtr;
#else
	typedef uint32 uintPtr;
	typedef sint32 sintPtr;
#endif

	namespace privat
	{
		template<bool>
		struct helper_is_char_signed
		{};

		template<>
		struct helper_is_char_signed<true>
		{
			typedef signed char type;
		};

		template<>
		struct helper_is_char_signed<false>
		{
			typedef unsigned char type;
		};
	}

	namespace detail
	{
		template<class T>
		struct numeric_limits
		{
			static const bool is_specialized = false;
		};

		template<>
		struct numeric_limits<unsigned char>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint8 min() { return 0; };
			static constexpr const uint8 max() { return 255u; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned short>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint16 min() { return 0; };
			static constexpr const uint16 max() { return 65535u; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned int>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint32 min() { return 0; };
			static constexpr const uint32 max() { return 4294967295u; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned long long>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint64 min() { return 0; };
			static constexpr const uint64 max() { return 18446744073709551615LLu; };
			typedef uint64 make_unsigned;
		};

#ifdef CAGE_SYSTEM_WINDOWS
		template<>
		struct numeric_limits<unsigned long> : public numeric_limits<unsigned int>
		{};
#else
		template<>
		struct numeric_limits<unsigned long> : public numeric_limits<unsigned long long>
		{};
#endif

		template<>
		struct numeric_limits<signed char>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint8 min() { return -127 - 1; };
			static constexpr const sint8 max() { return  127; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<signed short>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint16 min() { return -32767 - 1; };
			static constexpr const sint16 max() { return  32767; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<signed int>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint32 min() { return -2147483647 - 1; };
			static constexpr const sint32 max() { return  2147483647; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<signed long long>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint64 min() { return -9223372036854775807LL - 1; };
			static constexpr const sint64 max() { return  9223372036854775807LL; };
			typedef uint64 make_unsigned;
		};

#ifdef CAGE_SYSTEM_WINDOWS
		template<>
		struct numeric_limits<signed long> : public numeric_limits<signed int>
		{};
#else
		template<>
		struct numeric_limits<signed long> : public numeric_limits<signed long long>
		{};
#endif

		template<>
		struct numeric_limits<float>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const float min() { return -1e+37f; };
			static constexpr const float max() { return  1e+37f; };
			typedef float make_unsigned;
		};

		template<>
		struct numeric_limits<double>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const double min() { return -1e+308; };
			static constexpr const double max() { return  1e+308; };
			typedef double make_unsigned;
		};

		// char is distinct type from both unsigned char and signed char
		template<>
		struct numeric_limits<char> : public numeric_limits<privat::helper_is_char_signed<((signed char)-1) != ((unsigned char)-1)>::type>
		{};
	}

	namespace privat
	{
		namespace endianness
		{
			constexpr const uint32 one = 1;

			constexpr bool little()
			{
				return ((const uint8&)(one) == 1);
			}

			constexpr bool big() // network
			{
				return !little();
			}

			template<class T>
			constexpr T change(T val)
			{
				union U
				{
					T t;
					uint8 a[sizeof(T)];
					U() : t(T()) {}
				} u;
				u.t = val;
				for (uint32 i = 0; i < sizeof(T) / 2; i++)
				{
					uint8 tmp = u.a[i];
					u.a[i] = u.a[sizeof(T) - i - 1];
					u.a[sizeof(T) - i - 1] = tmp;
				}
				return u.t;
			}
		}

		struct maxStruct
		{
			template<class T>
			constexpr operator T () const
			{
				return detail::numeric_limits<T>::max();
			}
		};

		template<class T> constexpr bool operator == (T lhs, maxStruct rhs) { return lhs == detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator != (T lhs, maxStruct rhs) { return lhs != detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator <= (T lhs, maxStruct rhs) { return lhs <= detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator >= (T lhs, maxStruct rhs) { return lhs >= detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator < (T lhs, maxStruct rhs) { return lhs < detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator > (T lhs, maxStruct rhs) { return lhs > detail::numeric_limits<T>::max(); }
	}

	static constexpr const privat::maxStruct m = privat::maxStruct(); // represents the maximum value possible in any numeric type
}


