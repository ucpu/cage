#ifndef guard_containerSerialization_ncas56d74j8zud9s
#define guard_containerSerialization_ncas56d74j8zud9s

#include "serialization.h"

namespace cage
{
	namespace privat
	{
		template <class C, class N>
		auto reserveIfPossibleImpl(C &c, N n, int) -> decltype(c.reserve(n), void())
		{
			c.reserve(n);
		};

		template <class C, class N>
		auto reserveIfPossibleImpl(C &c, N n, float) -> void
		{};

		template <class C, class N>
		void reserveIfPossible(C &c, N n)
		{
			reserveIfPossibleImpl(c, n, 0);
		};
	}

	template<class C, typename std::enable_if_t<!std::is_trivially_copyable_v<C>, bool> = true>
	Serializer &operator << (Serializer &s, const C &c)
	{
		s << c.size();
		for (const auto &it : c)
			s << it;
		return s;
	}

	template<class C, typename std::enable_if_t<!std::is_trivially_copyable_v<C>, bool> = true>
	Deserializer &operator >> (Deserializer &s, C &c)
	{
		CAGE_ASSERT(c.empty());
		decltype(c.size()) size;
		s >> size;
		privat::reserveIfPossible(c, size);
		for (uint32 i = 0; i < size; i++)
		{
			typename C::value_type tmp;
			s >> tmp;
			c.insert(c.end(), std::move(tmp));
		}
		return s;
	}

	template<class A, class B>
	Serializer &operator << (Serializer &s, const std::pair<A, B> &p)
	{
		s << p.first << p.second;
		return s;
	}

	template<class A, class B>
	Deserializer &operator >> (Deserializer &s, std::pair<A, B> &p)
	{
		s >> const_cast<std::remove_cv_t<A> &>(p.first) >> const_cast<std::remove_cv_t<B> &>(p.second);
		return s;
	}
}

#endif
