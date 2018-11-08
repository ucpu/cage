#ifndef guard_serialization_h_edsg45df4h654fdr56h4gfd564h
#define guard_serialization_h_edsg45df4h654fdr56h4gfd564h

namespace cage
{
	namespace detail
	{
		template <class T>
		struct serializerArrayAccessor;
	}

	struct CAGE_API serializer
	{
		memoryBuffer &buffer;
		
		explicit serializer(memoryBuffer &buffer) : buffer(buffer)
		{}

		serializer &write(const void *data, uintPtr size);

		char *accessRaw(uintPtr size); // the pointer returned may be invalidated by any writes or other accesses to the serializer

		template <class T>
		detail::serializerArrayAccessor<T> accessArray(uint32 count);
	};

	template <class T>
	serializer &operator << (serializer &s, const T &v)
	{
		return s.write(&v, sizeof(T));
	}

	template <uint32 N>
	serializer &operator << (serializer &s, const detail::stringBase<N> &v)
	{
		uint32 l = v.length();
		return s.write(&l, sizeof(l)).write(v.c_str(), l);
	}

	template <class T>
	serializer &operator << (serializer &s, T *v)
	{
		CAGE_ASSERT_COMPILE(false, do_not_serialize_a_pointer);
	}

	template <class T>
	serializer &operator << (serializer &s, const holder<T> &v)
	{
		CAGE_ASSERT_COMPILE(false, do_not_serialize_holder);
	}

	namespace detail
	{
		template <class T>
		struct serializerArrayAccessor
		{
			serializerArrayAccessor(serializer &ser, char *ptr, uint32 count) : ser(ser), initOff(ptr - ser.buffer.data()), count(count)
			{}

			T &operator [] (uint32 idx)
			{
				CAGE_ASSERT_RUNTIME(idx < count);
				return accessRaw()[idx];
			}

			T *accessRaw()
			{
				return (T*)(ser.buffer.data() + initOff);
			}

		private:
			serializer &ser;
			const uintPtr initOff;
			const uint32 count;
		};
	}

	template <class T>
	detail::serializerArrayAccessor<T> serializer::accessArray(uint32 count)
	{
		return detail::serializerArrayAccessor<T>(*this, accessRaw(count * sizeof(T)), count);
	}

	struct CAGE_API deserializer
	{
		const char *current;
		const char *end;

		deserializer(const void *data, uintPtr size) : current((char*)data), end(current + size)
		{}

		explicit deserializer(const memoryBuffer &buffer);

		deserializer &read(void *data, uintPtr size)
		{
			if (current + size > end)
				CAGE_THROW_ERROR(exception, "deserialization beyond range");
			detail::memcpy(data, current, size);
			current += size;
			return *this;
		}

		const char *access(uintPtr size)
		{
			if (current + size > end)
				CAGE_THROW_ERROR(exception, "deserialization beyond range");
			const char *c = current;
			current += size;
			return c;
		}
	};

	template <class T>
	deserializer &operator >> (deserializer &s, T &v)
	{
		return s.read(&v, sizeof(T));
	}

	template <uint32 N>
	deserializer &operator >> (deserializer &s, detail::stringBase<N> &v)
	{
		uint32 l;
		s >> l;
		if (s.current + l > s.end)
			CAGE_THROW_ERROR(exception, "deserialization beyond range");
		v = detail::stringBase<N>(s.current, l);
		s.current += l;
		return s;
	}

	template <class T>
	deserializer &operator >> (deserializer &s, T *v)
	{
		CAGE_ASSERT_COMPILE(false, do_not_deserialize_a_pointer);
	}
}

#endif
