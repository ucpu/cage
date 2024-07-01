#ifndef guard_any_h_dfhg16awsetg4
#define guard_any_h_dfhg16awsetg4

#include <cage-core/typeIndex.h>

namespace cage
{
	namespace detail
	{
		template<class T, uint32 MaxSize>
		concept AnyValueConcept = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T> && std::is_same_v<std::remove_cvref_t<T>, T> && sizeof(T) <= MaxSize && sizeof(T) > 0;

		template<uint32 MaxSize_>
		struct alignas(16) AnyBase
		{
			AnyBase() = default;
			AnyBase(const AnyBase &) = default;

			template<AnyValueConcept<MaxSize_> T>
			CAGE_FORCE_INLINE AnyBase(const T &v)
			{
				detail::typeIndex<T>(); // detect hash collisions
				detail::memcpy(data_, &v, sizeof(T));
				type_ = detail::typeHash<T>();
			}

			AnyBase &operator=(const AnyBase &) = default;

			template<AnyValueConcept<MaxSize_> T>
			CAGE_FORCE_INLINE AnyBase &operator=(const T &v)
			{
				return *this = AnyBase(v);
			}

			template<AnyValueConcept<MaxSize_> T>
			CAGE_FORCE_INLINE bool has() const
			{
				return detail::typeHash<T>() == type_;
			}

			template<AnyValueConcept<MaxSize_> T>
			T get() const
			{
				CAGE_ASSERT(detail::typeHash<T>() == type_);
				T tmp;
				detail::memcpy(&tmp, data_, sizeof(T));
				return tmp;
			}

			CAGE_FORCE_INLINE void clear() { type_ = m; }
			CAGE_FORCE_INLINE uint32 typeHash() const { return type_; }
			CAGE_FORCE_INLINE explicit operator bool() const { return type_ != m; }
			static constexpr uint32 MaxSize = MaxSize_;

		private:
			char data_[MaxSize];
			uint32 type_ = m;
		};
	}
}

#endif // guard_any_h_dfhg16awsetg4
