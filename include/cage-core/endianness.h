#ifndef guard_endianness_h_l4kjhgsa8567fhj4hgfj
#define guard_endianness_h_l4kjhgsa8567fhj4hgfj

namespace cage
{
	namespace endianness
	{
		namespace privat
		{
			constexpr const uint32 one = 1;
		}

		constexpr bool little()
		{
			return ((const uint8&)(privat::one) == 1);
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
}

#endif // guard_endianness_h_l4kjhgsa8567fhj4hgfj
