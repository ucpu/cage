
#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	namespace
	{
		memoryBuffer dummy; // used for the reference when serializing directly into raw memory
	}

	serializer::serializer(void *data, uintPtr size) : buffer(dummy), data(data), offset(0), size(size)
	{}

	serializer::serializer(memoryBuffer &buffer, uintPtr size) : buffer(buffer), data(nullptr), offset(buffer.size()), size(buffer.size() + size)
	{}

	serializer::serializer(memoryBuffer &buffer, void *data, uintPtr offset, uintPtr size) : buffer(buffer), data(data), offset(offset), size(size)
	{}

	serializer serializer::placeholder(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return serializer(buffer, data, o, offset);
	}

	void serializer::write(const void *d, uintPtr s)
	{
		void *dst = advance(s);
		detail::memcpy(dst, d, s);
	}

	void *serializer::advance(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(exception, "serialization beyond available space");
		if (!data)
		{
			if (buffer.size() < offset + s)
				buffer.resizeSmart(offset + s);
		}
		void *dst = (char*)(data ? data : buffer.data()) + offset;
		offset += s;
		return dst;
	}

	uintPtr serializer::available() const
	{
		CAGE_ASSERT_RUNTIME(size >= offset);
		return size - offset;
	}

	deserializer::deserializer(const void *data, uintPtr size) : data(data), offset(0), size(size)
	{}

	deserializer::deserializer(const memoryBuffer &buffer) : data(buffer.data()), offset(0), size(buffer.size())
	{}

	deserializer::deserializer(const void *data, uintPtr offset, uintPtr size) : data(data), offset(offset), size(size)
	{}

	deserializer deserializer::placeholder(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return deserializer(data, o, offset);
	}

	void deserializer::read(void *d, uintPtr s)
	{
		const void *dst = advance(s);
		detail::memcpy(d, dst, s);
	}

	const void *deserializer::advance(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(exception, "deserialization beyond available space");
		const void *dst = (const char*)data + offset;
		offset += s;
		return dst;
	}

	uintPtr deserializer::available() const
	{
		CAGE_ASSERT_RUNTIME(size >= offset);
		return size - offset;
	}
}
