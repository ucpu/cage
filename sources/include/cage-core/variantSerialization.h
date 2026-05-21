#ifndef guard_variantSerialization_sreter74tz5
#define guard_variantSerialization_sreter74tz5

#include <cage-core/serialization.h>

namespace cage
{
	template<class T>
	requires(privat::IsStdVariant<T>::value)
	Serializer &operator<<(Serializer &s, const T &v)
	{
		s << numeric_cast<uint32>(v.index());
		std::visit([&](const auto &alt) { s << alt; }, v);
		return s;
	}

	namespace privat
	{
		template<std::size_t I, class Variant>
		void readVariantAlternative(Deserializer &s, Variant &v)
		{
			using Alt = std::variant_alternative_t<I, Variant>;
			Alt value;
			s >> value;
			v = std::move(value);
		}

		template<class Variant, std::size_t... Is>
		void readVariantByIndex(Deserializer &s, Variant &v, std::size_t idx, std::index_sequence<Is...>)
		{
			using Fnc = void (*)(Deserializer &, Variant &);
			static const Fnc table[] = { &readVariantAlternative<Is, Variant>... };
			table[idx](s, v);
		}
	}

	template<class T>
	requires(privat::IsStdVariant<T>::value)
	Deserializer &operator>>(Deserializer &s, T &v)
	{
		uint32 idx = 0;
		s >> idx;
		privat::readVariantByIndex(s, v, idx, std::make_index_sequence<std::variant_size_v<T>>{});
		return s;
	}
}

#endif
