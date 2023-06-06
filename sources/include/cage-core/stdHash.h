#ifndef guard_stdHash_ik4j1hb8vsaerg
#define guard_stdHash_ik4j1hb8vsaerg

#include "hashBuffer.h"

#include <functional> // std::hash

namespace cage
{
	template<class T>
	bool operator==(const cage::Holder<T> &a, const cage::Holder<T> &b) noexcept
	{
		return +a == +b;
	}
}

namespace std
{
	template<cage::uint32 N>
	struct hash<cage::detail::StringBase<N>>
	{
		std::size_t operator()(const cage::detail::StringBase<N> &s) const noexcept { return cage::hashBuffer(s); }
	};

	template<class T>
	struct hash<cage::Holder<T>>
	{
		std::size_t operator()(const cage::Holder<T> &v) const noexcept { return impl(v, 0); }

	private:
		template<class M>
		auto impl(const cage::Holder<M> &v, int) const noexcept -> decltype(std::hash<M>()(*v), std::size_t())
		{
			if (v)
				return std::hash<M>()(*v);
			return 0;
		}

		template<class M>
		auto impl(const cage::Holder<M> &v, float) const noexcept -> std::size_t
		{
			return std::hash<M *>()(+v);
		}
	};
}

#endif // guard_stdHash_ik4j1hb8vsaerg
