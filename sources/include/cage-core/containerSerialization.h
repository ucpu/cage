#ifndef guard_containerSerialization_ncas56d74j8zud9s
#define guard_containerSerialization_ncas56d74j8zud9s

#include "serialization.h"

namespace cage
{
	namespace privat
	{
		template<class C>
		concept ContainerConcept = requires(C c) {
			c.size();
			c.begin();
			c.end();
		};

		template<class C>
		concept ReservableContainerConcept = ContainerConcept<C> && requires(C c) {
			c.reserve();
		};
	}

	Serializer &operator << (Serializer &s, const privat::ContainerConcept auto &c)
	{
		s << numeric_cast<uint64>(c.size());
		for (const auto &it : c)
			s << it;
		return s;
	}

	Deserializer &operator >> (Deserializer &s, privat::ContainerConcept auto &c)
	{
		CAGE_ASSERT(c.empty());
		uint64 size = 0;
		s >> size;
		if constexpr (privat::ReservableContainerConcept<decltype(c)>)
			c.reserve(numeric_cast<decltype(c.size())>(size));
		for (uint32 i = 0; i < size; i++)
		{
			using Tmp = std::remove_cv_t<std::remove_reference_t<decltype(*c.begin())>>;
			Tmp tmp;
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
