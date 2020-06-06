#ifndef guard_serialization_h_edsg45df4h654fdr56h4gfd564h
#define guard_serialization_h_edsg45df4h654fdr56h4gfd564h

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API Serializer
	{
		explicit Serializer(PointerRange<char> buffer);
		explicit Serializer(MemoryBuffer &buffer, uintPtr size = m);
		
		Serializer(Serializer &&) = default;
		Serializer(const Serializer &) = delete;
		Serializer &operator = (Serializer &&) = default;
		Serializer &operator = (const Serializer &) = delete;

		uintPtr available() const; // number of bytes still available in the buffer (valid only if the maximum size was given in the constructor)
		Serializer placeholder(uintPtr s);
		void write(PointerRange<const char> buffer);
		[[deprecated]] void write(const void *d, uintPtr s);
		[[deprecated]] void write(const char *d, uintPtr s);
		char *advance(uintPtr s); // returns the original position

	private:
		explicit Serializer(MemoryBuffer *buffer, char *data, uintPtr offset, uintPtr size);

		MemoryBuffer *buffer = nullptr;
		char *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	struct CAGE_CORE_API Deserializer
	{
		explicit Deserializer(PointerRange<const char> buffer);

		Deserializer(Deserializer &&) = default;
		Deserializer(const Deserializer &) = delete;
		Deserializer &operator = (Deserializer &&) = default;
		Deserializer &operator = (const Deserializer &) = delete;

		uintPtr available() const; // number of bytes still available in the buffer
		Deserializer placeholder(uintPtr s);
		void read(PointerRange<char> buffer);
		[[deprecated]] void read(void *d, uintPtr s);
		[[deprecated]] void read(char *d, uintPtr s);
		const char *advance(uintPtr s); // returns the original position

	private:
		explicit Deserializer(const char *data, uintPtr offset, uintPtr size);

		const char *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	// helpers

	template<class T>
	constexpr PointerRange<char> bytesView(T &value)
	{
		static_assert(std::is_trivially_copyable<T>::value, "type not trivial");
		return PointerRange<char>(reinterpret_cast<char *>(&value), reinterpret_cast<char *>(&value + 1));
	}

	template<class T>
	constexpr PointerRange<const char> bytesView(const T &value)
	{
		static_assert(std::is_trivially_copyable<T>::value, "type not trivial");
		return PointerRange<const char>(reinterpret_cast<const char *>(&value), reinterpret_cast<const char *>(&value + 1));
	}

	// general serialization

	template<class T>
	Serializer &operator << (Serializer &s, const T &v)
	{
		s.write(bytesView(v));
		return s;
	}

	template<class T>
	Deserializer &operator >> (Deserializer &s, T &v)
	{
		s.read(bytesView(v));
		return s;
	}

	// specialization for strings

	template<uint32 N>
	Serializer &operator << (Serializer &s, const detail::StringBase<N> &v)
	{
		Serializer ss = s.placeholder(sizeof(uint32) + v.length()); // write all or nothing
		ss << v.length();
		ss.write({ v.c_str(), v.c_str() + v.length() });
		return s;
	}

	template<uint32 N>
	Deserializer &operator >> (Deserializer &s, detail::StringBase<N> &v)
	{
		decltype(v.length()) size;
		s >> size;
		v = detail::StringBase<N>((const char*)s.advance(size), size);
		return s;
	}

	// disable serialization of raw pointers

	template<class T>
	Serializer &operator << (Serializer &, const T *) = delete;

	template<class T>
	Deserializer &operator >> (Deserializer &, T *) = delete;
}

#endif
