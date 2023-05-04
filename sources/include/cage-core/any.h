#ifndef guard_any_h_dfhg16awsetg4
#define guard_any_h_dfhg16awsetg4

#include "typeIndex.h"

namespace cage
{
	namespace detail
	{
		template<uint32 MaxSize>
		struct alignas(16) AnyBase
		{
			AnyBase() = default;
			AnyBase(const AnyBase &) = default;

			template<class T>
			CAGE_FORCE_INLINE AnyBase(const T &v) noexcept
			{
				static_assert(std::is_trivially_copyable_v<T>);
				static_assert(sizeof(T) <= MaxSize);
				static_assert(sizeof(T) > 0);
				detail::typeIndex<T>(); // detect hash collisions
				detail::memcpy(data_, &v, sizeof(T));
				type_ = detail::typeHash<T>();
			}

			AnyBase &operator = (const AnyBase &) = default;

			template<class T>
			CAGE_FORCE_INLINE AnyBase &operator = (const T &v) noexcept
			{
				return *this = AnyBase(v);
			}

			CAGE_FORCE_INLINE void clear() noexcept
			{
				type_ = m;
			}

			CAGE_FORCE_INLINE uint32 typeHash() const noexcept
			{
				return type_;
			}

			CAGE_FORCE_INLINE explicit operator bool() const noexcept
			{
				return type_ != m;
			}

			template<class T>
			T get() const
			{
				static_assert(std::is_trivially_copyable_v<T>);
				static_assert(sizeof(T) <= MaxSize);
				static_assert(sizeof(T) > 0);
				CAGE_ASSERT(detail::typeHash<T>() == type_);
				T tmp;
				detail::memcpy(&tmp, data_, sizeof(T));
				return tmp;
			}

		private:
			char data_[MaxSize];
			uint32 type_ = m;
		};
	}
}

#endif // guard_any_h_dfhg16awsetg4
