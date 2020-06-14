#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/lineReader.h>

namespace cage
{
	Serializer::Serializer(PointerRange<char> buffer) : data(buffer.data()), size(buffer.size())
	{}

	Serializer::Serializer(MemoryBuffer &buffer, uintPtr size) : buffer(&buffer), size(size == m && buffer.size() != 0 ? buffer.size() : size)
	{}

	Serializer::Serializer(MemoryBuffer *buffer, char *data, uintPtr offset, uintPtr size) : buffer(buffer), data(data), offset(offset), size(size)
	{}

	Serializer Serializer::placeholder(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return Serializer(buffer, data, o, offset);
	}

	void Serializer::write(PointerRange<const char> buffer)
	{
		write(buffer.data(), buffer.size());
	}

	void Serializer::writeLine(const string &line)
	{
		string d = line + "\n";
		write({ d.c_str(), d.c_str() + d.length() });
	}

	void Serializer::write(const void *d, uintPtr s)
	{
		void *dst = advance(s);
		detail::memcpy(dst, d, s);
	}

	void Serializer::write(const char *d, uintPtr s)
	{
		char *dst = advance(s);
		detail::memcpy(dst, d, s);
	}

	char *Serializer::advance(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(Exception, "serialization beyond available space");
		if (!data)
		{
			if (buffer->size() < offset + s)
				buffer->resizeSmart(offset + s);
		}
		char *dst = (data ? data : buffer->data()) + offset;
		offset += s;
		return dst;
	}

	uintPtr Serializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}

	Deserializer::Deserializer(PointerRange<const char> buffer) : data(buffer.data()), size(buffer.size())
	{}

	Deserializer::Deserializer(const char *data, uintPtr offset, uintPtr size) : data(data), offset(offset), size(size)
	{}

	Deserializer Deserializer::placeholder(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return Deserializer(data, o, offset);
	}

	void Deserializer::read(PointerRange<char> buffer)
	{
		read(buffer.data(), buffer.size());
	}

	bool Deserializer::readLine(string &line)
	{
		uintPtr a = available();
		const char *p = data + offset;
		if (detail::readLine(line, p, a, false))
		{
			offset = p - data;
			return true;
		}
		return false;
	}

	void Deserializer::read(void *d, uintPtr s)
	{
		const void *src = advance(s);
		detail::memcpy(d, src, s);
	}

	void Deserializer::read(char *d, uintPtr s)
	{
		const char *src = advance(s);
		detail::memcpy(d, src, s);
	}

	const char *Deserializer::advance(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(Exception, "deserialization beyond available space");
		const char *dst = (const char*)data + offset;
		offset += s;
		return dst;
	}

	uintPtr Deserializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}
}
