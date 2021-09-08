#ifndef guard_serialization_h_edsg45df4h654fdr56h4gfd564h
#define guard_serialization_h_edsg45df4h654fdr56h4gfd564h

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API Serializer : private Noncopyable
	{
		explicit Serializer(PointerRange<char> buffer);
		explicit Serializer(MemoryBuffer &buffer, uintPtr size = m);
		
		uintPtr available() const; // number of bytes still available in the buffer (valid only if the maximum size was given in the constructor)
		void write(PointerRange<const char> buffer);
		void writeLine(const String &line);
		Serializer reserve(uintPtr s);

	private:
		explicit Serializer(MemoryBuffer *buffer, char *data, uintPtr offset, uintPtr size);
		PointerRange<char> advance(uintPtr s); // future writes may cause the MemoryBuffer to reallocate invalidating the PointerRange

		MemoryBuffer *buffer = nullptr;
		char *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	struct CAGE_CORE_API Deserializer : private Noncopyable
	{
		explicit Deserializer(PointerRange<const char> buffer);

		uintPtr available() const; // number of bytes still available in the buffer
		void read(PointerRange<char> buffer);
		PointerRange<const char> read(uintPtr size);
		bool readLine(PointerRange<const char> &line);
		bool readLine(String &line);
		Deserializer subview(uintPtr s);
		Deserializer copy() const;

	private:
		explicit Deserializer(const char *data, uintPtr offset, uintPtr size);

		const char *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	// helpers

	// reinterpret types of range of elements
	// preserves number of bytes, but may change number of elements
	template<class Dst = const char, class Src>
	constexpr PointerRange<Dst> bufferCast(const PointerRange<Src> src)
	{
		static_assert(std::is_trivially_copyable_v<Dst>);
		static_assert(std::is_trivially_copyable_v<Src>);
		return { reinterpret_cast<Dst *>(src.begin()), reinterpret_cast<Dst *>(src.end()) };
	}

	// make range of a single object
	template<class T>
	constexpr PointerRange<T> rangeView(T &object)
	{
		return { &object, &object + 1 };
	}

	// reinterpret single object as range of bytes
	template<class Dst = const char, class Src>
	constexpr PointerRange<Dst> bufferView(Src &object)
	{
		return bufferCast<Dst, Src>(rangeView<Src>(object));
	}

	// general serialization

	template<class T, typename std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
	Serializer &operator << (Serializer &s, const T &v)
	{
		static_assert(!std::is_pointer_v<T>, "pointer serialization is forbidden");
		s.write(bufferView<const char>(v));
		return s;
	}

	template<class T, typename std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
	Deserializer &operator >> (Deserializer &s, T &v)
	{
		static_assert(!std::is_pointer_v<T>, "pointer serialization is forbidden");
		s.read(bufferView<char>(v));
		return s;
	}

	// r-value deserializer

	/*
	namespace privat
	{
		template <class T>
		auto isDeserializableImpl(Deserializer &s, T &v, int) -> decltype(s >> v, int())
		{};

		template <class T>
		auto isDeserializableImpl(Deserializer &s, T &v, float) -> void
		{};

		template <class T>
		auto isDeserializableImpl()
		{
			Deserializer s({});
			T v;
			return isDeserializableImpl<T>(s, v, 0);
		};

		template <class T>
		struct isDeserializable
		{
			constexpr static bool value = std::is_same_v<decltype(isDeserializableImpl<T>()), int>;
		};
	}

	template<class T, typename std::enable_if_t<privat::isDeserializable<T>::value, bool> = true>
	Deserializer &&operator >> (Deserializer &&s, T &v)
	{
		s >> v;
		return std::move(s);
	}
	*/

	template<class T>
	Deserializer &&operator >> (Deserializer &&s, T &v)
	{
		s >> v;
		return std::move(s);
	}

	// overloads for c array

	template<class T, uintPtr N>
	Serializer &operator << (Serializer &s, const T (&v)[N])
	{
		for (auto &it : v)
			s << it;
		return s;
	}

	template<class T, uintPtr N>
	Deserializer &operator >> (Deserializer &s, T (&v)[N])
	{
		for (auto &it : v)
			s >> it;
		return s;
	}

	// overloads for strings

	template<uint32 N>
	Serializer &operator << (Serializer &s, const detail::StringBase<N> &v)
	{
		s << v.length();
		s.write(v);
		return s;
	}

	template<uint32 N>
	Deserializer &operator >> (Deserializer &s, detail::StringBase<N> &v)
	{
		decltype(v.length()) size;
		s >> size;
		v = detail::StringBase<N>(s.read(size));
		return s;
	}
}

#endif
