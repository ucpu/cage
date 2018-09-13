#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/stdBufferStream.h>

namespace cage
{
	bufferIStream::bufferIStream(const void *data, uintPtr size) : std::istream(this)
	{
		setg((char*)data, (char*)data, (char*)data + size);
		exceptions(std::istream::badbit | std::istream::failbit);
	}

	bufferIStream::bufferIStream(const memoryBuffer &buffer) : bufferIStream(buffer.data(), buffer.size())
	{}

	uintPtr bufferIStream::position() const
	{
		return gptr() - eback();
	}

	bufferOStream::bufferOStream(memoryBuffer &buffer) : std::ostream(this), buffer(buffer)
	{
		exceptions(std::istream::badbit | std::istream::failbit);
	}

	std::streamsize bufferOStream::xsputn(const char* s, std::streamsize n)
	{
		uintPtr origSize = buffer.size();
		buffer.resizeGrowSmart(origSize + n);
		detail::memcpy(buffer.data() + origSize, s, n);
		return n;
	};

	bufferOStream::int_type bufferOStream::overflow(int_type ch)
	{
		char c = (char)ch;
		return (int)xsputn(&c, 1);
	}
}
