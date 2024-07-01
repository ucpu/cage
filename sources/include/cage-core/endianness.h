#ifndef guard_endianness_h_l4kjhgsa8567fhj4hgfj
#define guard_endianness_h_l4kjhgsa8567fhj4hgfj

#include <cage-core/core.h>

namespace cage
{
	namespace endianness
	{
		CAGE_FORCE_INLINE constexpr uint8 change(uint8 val)
		{
			return val;
		}
		CAGE_FORCE_INLINE constexpr uint16 change(uint16 val)
		{
			return (val & 0xff00) >> 8 | (val & 0x00ff) << 8;
		}
		CAGE_FORCE_INLINE constexpr uint32 change(uint32 val)
		{
			uint32 a = change(uint16(val));
			uint32 b = change(uint16(val >> 16));
			return a << 16 | b;
		}
		CAGE_FORCE_INLINE constexpr uint64 change(uint64 val)
		{
			uint64 a = change(uint32(val));
			uint64 b = change(uint32(val >> 32));
			return a << 32 | b;
		}

		template<class T>
		CAGE_FORCE_INLINE constexpr T change(T val)
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
