#ifndef guard_stdHash_ik4j1hb8vsaerg
#define guard_stdHash_ik4j1hb8vsaerg

#include <functional> // std::hash

#include <cage-core/hashBuffer.h>

namespace cage
{
	template<class T>
	bool operator==(const cage::Holder<T> &a, const cage::Holder<T> &b)
	{
		return +a == +b;
	}
}

namespace std
{
	template<cage::uint32 N>
	struct hash<cage::detail::StringBase<N>>
	{
		std::size_t operator()(const cage::detail::StringBase<N> &s) const { return cage::hashBuffer(s); }
	};

	template<class T>
	struct hash<cage::Holder<T>>
	{
		std::size_t operator()(const cage::Holder<T> &v) const
		{
			if (!v)
				return 0;
			if constexpr (std::is_same_v<std::remove_cv_t<T>, void>)
			{
				return std::hash<std::size_t>{}(std::size_t(+v));
			}
			else if constexpr (requires(const T &v) {
								   { std::hash<T>{}(v) } -> std::convertible_to<std::size_t>;
							   })
			{
				return std::hash<T>{}(*v);
			}
			else
			{
				return std::hash<T *>{}(+v);
			}
		}
	};
}

#endif // guard_stdHash_ik4j1hb8vsaerg
