#ifndef guard_endianness_h_l4kjhgsa8567fhj4hgfj
#define guard_endianness_h_l4kjhgsa8567fhj4hgfj

#include "core.h"

namespace cage
{
	namespace endianness
	{
		CAGE_FORCE_INLINE consteval bool little() noexcept
		{
			const uint32 one = 1;
			return (const uint8 &)one == 1;
		}

		CAGE_FORCE_INLINE consteval bool big() noexcept // network
		{
			return !little();
		}

		template<class T>
		CAGE_FORCE_INLINE constexpr T change(T val) noexcept
		{
			static_assert(std::is_trivially_copyable_v<T>);
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
