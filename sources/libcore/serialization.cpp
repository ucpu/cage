
#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	Serializer::Serializer(void *data, uintPtr size) : buffer(nullptr), data(data), offset(0), size(size)
	{}

	Serializer::Serializer(MemoryBuffer &buffer, uintPtr size) : buffer(&buffer), data(nullptr), offset(buffer.size()), size(buffer.size() + size)
	{}

	Serializer::Serializer(MemoryBuffer *buffer, void *data, uintPtr offset, uintPtr size) : buffer(buffer), data(data), offset(offset), size(size)
	{}

	Serializer Serializer::placeholder(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return Serializer(buffer, data, o, offset);
	}

	void Serializer::write(const void *d, uintPtr s)
	{
		void *dst = advance(s);
		detail::memcpy(dst, d, s);
	}

	void *Serializer::advance(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(Exception, "serialization beyond available space");
		if (!data)
		{
			if (buffer->size() < offset + s)
				buffer->resizeSmart(offset + s);
		}
		void *dst = (char*)(data ? data : buffer->data()) + offset;
		offset += s;
		return dst;
	}

	uintPtr Serializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}

	Deserializer::Deserializer(const void *data, uintPtr size) : data(data), offset(0), size(size)
	{}

	Deserializer::Deserializer(const MemoryBuffer &buffer) : data(buffer.data()), offset(0), size(buffer.size())
	{}

	Deserializer::Deserializer(const void *data, uintPtr offset, uintPtr size) : data(data), offset(offset), size(size)
	{}

	Deserializer Deserializer::placeholder(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return Deserializer(data, o, offset);
	}

	void Deserializer::read(void *d, uintPtr s)
	{
		const void *dst = advance(s);
		detail::memcpy(d, dst, s);
	}

	const void *Deserializer::advance(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(Exception, "deserialization beyond available space");
		const void *dst = (const char*)data + offset;
		offset += s;
		return dst;
	}

	uintPtr Deserializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}
}
