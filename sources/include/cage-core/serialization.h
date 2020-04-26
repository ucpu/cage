#ifndef guard_serialization_h_edsg45df4h654fdr56h4gfd564h
#define guard_serialization_h_edsg45df4h654fdr56h4gfd564h

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API Serializer
	{
		explicit Serializer(void *data, uintPtr size);
		explicit Serializer(MemoryBuffer &buffer, uintPtr size = m);
		
		Serializer(Serializer &&) = default;
		Serializer(const Serializer &) = delete;
		Serializer &operator = (Serializer &&) = default;
		Serializer &operator = (const Serializer &) = delete;

		uintPtr available() const; // number of bytes still available in the buffer (valid only if the maximum size was given in the constructor)
		Serializer placeholder(uintPtr s);
		void write(const void *d, uintPtr s);
		void *advance(uintPtr s); // returns the original position

	private:
		explicit Serializer(MemoryBuffer *buffer, void *data, uintPtr offset, uintPtr size);

		MemoryBuffer *buffer = nullptr;
		void *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	struct CAGE_CORE_API Deserializer
	{
		explicit Deserializer(const void *data, uintPtr size);
		explicit Deserializer(const MemoryBuffer &buffer);

		Deserializer(Deserializer &&) = default;
		Deserializer(const Deserializer &) = delete;
		Deserializer &operator = (Deserializer &&) = default;
		Deserializer &operator = (const Deserializer &) = delete;

		uintPtr available() const; // number of bytes still available in the buffer
		Deserializer placeholder(uintPtr s);
		void read(void *d, uintPtr s);
		const void *advance(uintPtr s); // returns the original position

	private:
		explicit Deserializer(const void *data, uintPtr offset, uintPtr size);

		const void *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	// general serialization

	template<class T>
	Serializer &operator << (Serializer &s, const T &v)
	{
		static_assert(std::is_trivially_copyable<T>::value, "type not trivial");
		s.write(&v, sizeof(v));
		return s;
	}

	template<class T>
	Deserializer &operator >> (Deserializer &s, T &v)
	{
		static_assert(std::is_trivially_copyable<T>::value, "type not trivial");
		s.read(&v, sizeof(v));
		return s;
	}

	// specialization for strings

	template<uint32 N>
	Serializer &operator << (Serializer &s, const detail::StringBase<N> &v)
	{
		Serializer ss = s.placeholder(sizeof(v.length()) + v.length()); // write all or nothing
		ss << v.length();
		ss.write(v.c_str(), v.length());
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
	Serializer &operator << (Serializer &s, const T *v) = delete;

	template<class T>
	Deserializer &operator >> (Deserializer &s, T *v) = delete;
}

#endif
