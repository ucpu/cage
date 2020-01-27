#include <cage-core/memoryBuffer.h>
#include <cage-core/stdBufferStream.h>

namespace cage
{
	BufferIStream::BufferIStream(const void *data, uintPtr size) : std::istream(this)
	{
		setg((char*)data, (char*)data, (char*)data + size);
		exceptions(std::istream::badbit | std::istream::failbit);
	}

	BufferIStream::BufferIStream(const MemoryBuffer &buffer) : BufferIStream(buffer.data(), buffer.size())
	{}

	uintPtr BufferIStream::position() const
	{
		return gptr() - eback();
	}

	BufferOStream::BufferOStream(MemoryBuffer &buffer) : std::ostream(this), buffer(buffer)
	{
		exceptions(std::istream::badbit | std::istream::failbit);
	}

	std::streamsize BufferOStream::xsputn(const char* s, std::streamsize n)
	{
		uintPtr origSize = buffer.size();
		buffer.resizeSmart(numeric_cast<uintPtr>(origSize + n));
		detail::memcpy(buffer.data() + origSize, s, numeric_cast<uintPtr>(n));
		return n;
	};

	BufferOStream::int_type BufferOStream::overflow(int_type ch)
	{
		char c = (char)ch;
		return (int)xsputn(&c, 1);
	}
}
