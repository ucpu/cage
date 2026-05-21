#ifndef guard_containerSerialization_ncas56d74j8zud9s
#define guard_containerSerialization_ncas56d74j8zud9s

#include <cage-core/serialization.h>

namespace cage
{
	namespace serialization
	{
		template<class T>
		struct Memcpyable : std::bool_constant<std::is_arithmetic_v<T>>
		{};

		template<>
		struct Memcpyable<Real> : std::true_type
		{};
		template<>
		struct Memcpyable<Vec2> : std::true_type
		{};
		template<>
		struct Memcpyable<Vec2i> : std::true_type
		{};
		template<>
		struct Memcpyable<Vec3> : std::true_type
		{};
		template<>
		struct Memcpyable<Vec3i> : std::true_type
		{};
		template<>
		struct Memcpyable<Vec4> : std::true_type
		{};
		template<>
		struct Memcpyable<Vec4i> : std::true_type
		{};
		template<>
		struct Memcpyable<Quat> : std::true_type
		{};
		template<>
		struct Memcpyable<Mat3> : std::true_type
		{};
		template<>
		struct Memcpyable<Mat4> : std::true_type
		{};
		template<>
		struct Memcpyable<Mat3x4> : std::true_type
		{};
		template<>
		struct Memcpyable<Transform> : std::true_type
		{};

		template<>
		struct Memcpyable<Line> : std::true_type
		{};
		template<>
		struct Memcpyable<Triangle> : std::true_type
		{};
		template<>
		struct Memcpyable<Plane> : std::true_type
		{};
		template<>
		struct Memcpyable<Sphere> : std::true_type
		{};
		template<>
		struct Memcpyable<Aabb> : std::true_type
		{};
		template<>
		struct Memcpyable<Cone> : std::true_type
		{};
		template<>
		struct Memcpyable<Frustum> : std::true_type
		{};
	}

	namespace privat
	{
		template<class T>
		concept MemcpyableConcept = serialization::Memcpyable<T>::value;

		template<class C>
		concept WritableContainerConcept = requires(C c) {
			c.size();
			c.begin();
			c.end();
			c.empty();
			typename C::value_type;
			c.insert(c.end(), *c.begin());
		} && SerializableConcept<typename C::value_type>;

		template<class C>
		concept MemcpyContainerConcept = WritableContainerConcept<C> && requires(C c) { c.data(); } && MemcpyableConcept<typename C::value_type>;
	}

	template<class Cont>
	requires(privat::WritableContainerConcept<Cont>)
	Serializer &operator<<(Serializer &s, const Cont &c)
	{
		s << numeric_cast<uint64>(c.size());
		if constexpr (std::is_same_v<std::remove_cvref_t<typename Cont::value_type>, bool>)
		{
			const uint64 bytes = c.size() / 8;
			for (uint64 i = 0; i < bytes; i++)
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
	requires(privat::WritableContainerConcept<Cont>)
	Deserializer &operator>>(Deserializer &s, Cont &c)
	{
		CAGE_ASSERT(c.empty());
		uint64 size = 0;
		s >> size;
		if constexpr (requires(Cont c) { c.reserve(42); })
		{
			c.reserve(numeric_cast<decltype(c.size())>(size));
		}
		if constexpr (std::is_same_v<std::remove_cvref_t<typename Cont::value_type>, bool>)
		{
			const uint64 bytes = size / 8;
			for (uint64 i = 0; i < bytes; i++)
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
		else if constexpr (privat::MemcpyContainerConcept<Cont> && requires(Cont c) { c.resize(42); })
		{
			c.resize(numeric_cast<decltype(c.size())>(size));
			s.readInto(bufferCast<char>(PointerRange<typename Cont::value_type>(c)));
		}
		else
		{
			using Tmp = std::remove_cv_t<std::remove_reference_t<decltype(*c.begin())>>;
			Tmp tmp;
			for (uint64 i = 0; i < size; i++)
			{
				s >> tmp;
				c.insert(c.end(), std::move(tmp));
			}
		}
		return s;
	}

	// overload for pointer range

	template<class T>
	requires requires(Serializer s, T v) { s << v; } // requires <<, but not >>
	Serializer &operator<<(Serializer &s, PointerRange<T> c)
	{
		s << numeric_cast<uint64>(c.size());
		if constexpr (privat::MemcpyableConcept<T>)
		{
			s.write(bufferCast(c));
		}
		else
		{
			for (const auto &v : c)
				s << v;
		}
		return s;
	}
}

#endif
