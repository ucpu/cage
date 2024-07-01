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
		std::size_t operator()(const cage::Holder<T> &v) const { return impl(v, 0); }

	private:
		template<class M>
		auto impl(const cage::Holder<M> &v, int) const -> decltype(std::hash<M>()(*v), std::size_t())
		{
			if (v)
				return std::hash<M>()(*v);
			return 0;
		}

		template<class M>
		auto impl(const cage::Holder<M> &v, float) const -> std::size_t
		{
			return std::hash<M *>()(+v);
		}
	};
}

#endif // guard_stdHash_ik4j1hb8vsaerg
