#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	namespace privat
	{
		SerializationInterface::SerializationInterface(PointerRange<char> buffer)
		{
			origin = buffer.data();
			getData = +[](void *origin) -> char * { return (char *)origin; };
		}

		SerializationInterface::SerializationInterface(MemoryBuffer &buffer)
		{
			origin = &buffer;
			getData = +[](void *origin) -> char * { return ((MemoryBuffer *)origin)->data(); };
			getSize = +[](void *origin) -> uintPtr { return ((MemoryBuffer *)origin)->size(); };
			setSize = +[](void *origin, uintPtr size) -> void { ((MemoryBuffer *)origin)->resizeSmart(size); };
		}
	}

	Serializer::Serializer(PointerRange<char> buffer) : interface(buffer), capacity(buffer.size()) {}

	Serializer::Serializer(MemoryBuffer &buffer, uintPtr capacity) : interface(buffer), capacity(capacity == m && buffer.size() != 0 ? buffer.size() : capacity) {}

	Serializer::Serializer(privat::SerializationInterface interface, uintPtr offset, uintPtr capacity) : interface(interface), offset(offset), capacity(capacity) {}

	uintPtr Serializer::available() const
	{
		CAGE_ASSERT(capacity >= offset);
		return capacity - offset;
	}

	void Serializer::write(PointerRange<const char> buffer)
	{
		if (buffer.empty())
			return;
		auto dst = write(buffer.size());
		detail::memcpy(dst.data(), buffer.data(), buffer.size());
	}

	PointerRange<char> Serializer::write(uintPtr size)
	{
		if (available() < size)
			CAGE_THROW_ERROR(Exception, "serialization beyond available space");
		if (interface.getSize && interface.getSize(interface.origin) < offset + size)
		{
			CAGE_ASSERT(interface.setSize);
			interface.setSize(interface.origin, offset + size);
		}
		char *dst = interface.getData(interface.origin) + offset;
		offset += size;
		return { dst, dst + size };
	}

	void Serializer::writeLine(const String &line)
	{
		write(line);
		write("\n");
	}

	Serializer Serializer::reserve(uintPtr s)
	{
		const uintPtr o = offset;
		write(s);
		return Serializer(interface, o, offset);
	}

	Deserializer::Deserializer(PointerRange<const char> buffer) : data(buffer.data()), size(buffer.size()) {}

	Deserializer::Deserializer(const char *data, uintPtr offset, uintPtr size) : data(data), offset(offset), size(size) {}

	uintPtr Deserializer::available() const
	{
		CAGE_ASSERT(size >= offset);
		return size - offset;
	}

	void Deserializer::read(PointerRange<char> buffer)
	{
		auto src = read(buffer.size());
		if (!buffer.empty())
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
		const uintPtr l = detail::readLine(line, pr, false);
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
		const uintPtr l = detail::readLine(line, pr, false);
		if (l)
		{
			offset += l;
			return true;
		}
		return false;
	}

	Deserializer Deserializer::subview(uintPtr s)
	{
		const uintPtr o = offset;
		read(s);
		return Deserializer(data, o, offset);
	}

	Deserializer Deserializer::copy() const
	{
		return Deserializer(data, offset, size);
	}
}
