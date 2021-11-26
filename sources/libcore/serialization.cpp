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

	uintPtr Serializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}

	void Serializer::write(PointerRange<const char> buffer)
	{
		auto dst = advance(buffer.size());
		detail::memcpy(dst.data(), buffer.data(), buffer.size());
	}

	void Serializer::writeLine(const String &line)
	{
		write(line);
		write("\n");
	}

	Serializer Serializer::reserve(uintPtr s)
	{
		uintPtr o = offset;
		advance(s);
		return Serializer(buffer, data, o, offset);
	}

	PointerRange<char> Serializer::advance(uintPtr s)
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
		return { dst, dst + s };
	}

	Deserializer::Deserializer(PointerRange<const char> buffer) : data(buffer.data()), size(buffer.size())
	{}

	Deserializer::Deserializer(const char *data, uintPtr offset, uintPtr size) : data(data), offset(offset), size(size)
	{}

	uintPtr Deserializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}

	void Deserializer::read(PointerRange<char> buffer)
	{
		auto src = read(buffer.size());
		detail::memcpy(buffer.data(), src.data(), buffer.size());
	}

	PointerRange<const char> Deserializer::read(uintPtr s)
	{
		if (available() < s)
			CAGE_THROW_ERROR(Exception, "deserialization beyond available space");
		const char *dst = data + offset;
		offset += s;
		return { dst, dst + s };
	}

	bool Deserializer::readLine(PointerRange<const char> &line)
	{
		const uintPtr a = available();
		PointerRange<const char> pr = { data + offset, data + offset + a };
		uintPtr l = detail::readLine(line, pr, false);
		if (l)
		{
			offset += l;
			return true;
		}
		return false;
	}

	bool Deserializer::readLine(String &line)
	{
		const uintPtr a = available();
		PointerRange<const char> pr = { data + offset, data + offset + a };
		uintPtr l = detail::readLine(line, pr, false);
		if (l)
		{
			offset += l;
			return true;
		}
		return false;
	}

	Deserializer Deserializer::subview(uintPtr s)
	{
		uintPtr o = offset;
		read(s);
		return Deserializer(data, o, offset);
	}

	Deserializer Deserializer::copy() const
	{
		return Deserializer(data, offset, size);
	}
}
