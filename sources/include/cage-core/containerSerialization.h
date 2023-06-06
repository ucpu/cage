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
			typename C::value_type;
		};

		template<class C>
		concept ReservableContainerConcept = ContainerConcept<C> && requires(C c) { c.reserve(42); };

		template<class C>
		concept InsertableContainerConcept = ContainerConcept<C> && requires(C c) { c.insert(c.end(), *c.begin()); };

		template<class C>
		concept IsCageString = requires(C c) {
			String(c);
			C(String());
		};

		template<class C>
		concept MemcpyContainerConcept = ContainerConcept<C> && std::is_trivially_copyable_v<std::remove_cvref_t<typename C::value_type>> && (!IsCageString<typename C::value_type>)&&requires(C c) { c.data(); };

		template<class C>
		concept MemcpyWritableContainerConcept = MemcpyContainerConcept<C> && requires(C c) { c.resize(42); };

		template<class C>
		concept ContainerOfBoolConcept = ContainerConcept<C> && std::is_same_v<std::remove_cvref_t<typename C::value_type>, bool>;
	}

	template<class Cont>
	requires(privat::ContainerConcept<Cont>)
	Serializer &operator<<(Serializer &s, const Cont &c)
	{
		s << numeric_cast<uint64>(c.size());
		if constexpr (privat::ContainerOfBoolConcept<Cont>)
		{
			const uint32 bytes = c.size() / 8;
			for (uint32 i = 0; i < bytes; i++)
			{
				uint8 b = 0;
				for (uint32 j = 0; j < 8; j++)
					b |= (1u << j) * (c[i * 8 + j]);
				s << b;
			}
			const uint32 rem = c.size() % 8;
			if (rem)
			{
				uint8 b = 0;
				for (uint32 j = 0; j < rem; j++)
					b |= (1u << j) * (c[bytes * 8 + j]);
				s << b;
			}
		}
		else if constexpr (privat::MemcpyContainerConcept<Cont>)
		{
			s.write(bufferCast(PointerRange<const typename Cont::value_type>(c)));
		}
		else
		{
			for (const auto &it : c)
				s << it;
		}
		return s;
	}

	template<class Cont>
	requires(privat::InsertableContainerConcept<Cont>)
	Deserializer &operator>>(Deserializer &s, Cont &c)
	{
		CAGE_ASSERT(c.empty());
		uint64 size = 0;
		s >> size;
		if constexpr (privat::ReservableContainerConcept<Cont>)
			c.reserve(numeric_cast<decltype(c.size())>(size));
		if constexpr (privat::ContainerOfBoolConcept<Cont>)
		{
			const uint32 bytes = size / 8;
			for (uint32 i = 0; i < bytes; i++)
			{
				uint8 b = 0;
				s >> b;
				for (uint32 j = 0; j < 8; j++)
					c.insert(c.end(), !!(b & (1u << j)));
			}
			const uint32 rem = size % 8;
			if (rem)
			{
				uint8 b = 0;
				s >> b;
				for (uint32 j = 0; j < rem; j++)
					c.insert(c.end(), !!(b & (1u << j)));
			}
		}
		else if constexpr (privat::MemcpyWritableContainerConcept<Cont>)
		{
			c.resize(numeric_cast<decltype(c.size())>(size));
			s.read(bufferCast<char>(PointerRange<typename Cont::value_type>(c)));
		}
		else
		{
			for (uint32 i = 0; i < size; i++)
			{
				using Tmp = std::remove_cv_t<std::remove_reference_t<decltype(*c.begin())>>;
				Tmp tmp;
				s >> tmp;
				c.insert(c.end(), std::move(tmp));
			}
		}
		return s;
	}

	template<class A, class B>
	Serializer &operator<<(Serializer &s, const std::pair<A, B> &p)
	{
		s << p.first << p.second;
		return s;
	}

	template<class A, class B>
	Deserializer &operator>>(Deserializer &s, std::pair<A, B> &p)
	{
		s >> const_cast<std::remove_cv_t<A> &>(p.first) >> const_cast<std::remove_cv_t<B> &>(p.second);
		return s;
	}
}

#endif
