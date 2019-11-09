#ifndef guard_serialization_h_edsg45df4h654fdr56h4gfd564h
#define guard_serialization_h_edsg45df4h654fdr56h4gfd564h

namespace cage
{
	struct CAGE_API serializer
	{
		explicit serializer(void *data, uintPtr size);
		explicit serializer(memoryBuffer &buffer, uintPtr size = m);
		
		serializer(serializer &&) = default;
		serializer(const serializer &) = delete;
		serializer &operator = (serializer &&) = default;
		serializer &operator = (const serializer &) = delete;

		uintPtr available() const; // number of bytes still available in the buffer (valid only if the maximum size was given in the constructor)
		serializer placeholder(uintPtr s);
		void write(const void *d, uintPtr s);
		void *advance(uintPtr s); // returns the original position

	private:
		explicit serializer(memoryBuffer *buffer, void *data, uintPtr offset, uintPtr size);

		memoryBuffer *buffer;
		void *data;
		uintPtr offset; // current position in the buffer
		uintPtr size; // max size of the buffer
	};

	struct CAGE_API deserializer
	{
		explicit deserializer(const void *data, uintPtr size);
		explicit deserializer(const memoryBuffer &buffer);

		deserializer(deserializer &&) = default;
		deserializer(const deserializer &) = delete;
		deserializer &operator = (deserializer &&) = default;
		deserializer &operator = (const deserializer &) = delete;

		uintPtr available() const; // number of bytes still available in the buffer
		deserializer placeholder(uintPtr s);
		void read(void *d, uintPtr s);
		const void *advance(uintPtr s); // returns the original position

	private:
		explicit deserializer(const void *data, uintPtr offset, uintPtr size);

		const void *data;
		uintPtr offset; // current position in the buffer
		uintPtr size; // max size of the buffer
	};

	// general serialization

	template<class T>
	serializer &operator << (serializer &s, const T &v)
	{
		s.write(&v, sizeof(v));
		return s;
	}

	template<class T>
	deserializer &operator >> (deserializer &s, T &v)
	{
		s.read(&v, sizeof(v));
		return s;
	}

	// specialization for strings

	template<uint32 N>
	serializer &operator << (serializer &s, const detail::stringBase<N> &v)
	{
		serializer ss = s.placeholder(sizeof(v.length()) + v.length()); // write all or nothing
		ss << v.length();
		ss.write(v.c_str(), v.length());
		return s;
	}

	template<uint32 N>
	deserializer &operator >> (deserializer &s, detail::stringBase<N> &v)
	{
		decltype(v.length()) size;
		s >> size;
		v = detail::stringBase<N>((const char*)s.advance(size), size);
		return s;
	}

	// disable serialization of raw pointers and holders

	template<class T>
	serializer &operator << (serializer &s, const T *v) = delete;

	template<class T>
	serializer &operator << (serializer &s, const holder<T> &v) = delete;

	template<class T>
	deserializer &operator >> (deserializer &s, T *v) = delete;

	template<class T>
	deserializer &operator >> (deserializer &s, holder<T> &v) = delete;
}

#endif
