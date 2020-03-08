#include <cage-core/memoryBuffer.h>
#include <cage-core/stdBufferStream.h>
#include <cage-core/math.h> // max

namespace cage
{
	BufferIStream::BufferIStream(const void *data, uintPtr size) : std::istream(this)
	{
		setg((char*)data, (char*)data, (char*)data + size);
		exceptions(std::istream::badbit | std::istream::failbit);
	}

	BufferIStream::BufferIStream(const MemoryBuffer &buffer) : BufferIStream(buffer.data(), buffer.size())
	{}

	BufferIStream::pos_type BufferIStream::seekpos(pos_type pos, std::ios_base::openmode which)
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	BufferIStream::pos_type BufferIStream::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
	{
		if ((which & ios_base::in) == ios_base::in)
		{
			char *s = eback(), *p = gptr(), *c = egptr();

			switch (dir)
			{
			case std::ios_base::beg:
				setg(s, s + off, c);
				break;
			case std::ios_base::cur:
				setg(s, p + off, c);
				break;
			case std::ios_base::end:
				setg(s, c + off, c);
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid seek direction");
			}
			return gptr() - eback();
		}
		return pos_type(off_type(-1));
	}

	BufferOStream::BufferOStream(MemoryBuffer &buffer) : std::ostream(this), buffer(buffer)
	{
		exceptions(std::istream::badbit | std::istream::failbit);
	}

	std::streamsize BufferOStream::xsputn(const char *s, std::streamsize n)
	{
		uintPtr req = writePos + n;
		if (buffer.size() < req)
			buffer.resizeSmart(req);
		detail::memcpy(buffer.data() + writePos, s, numeric_cast<uintPtr>(n));
		writePos += n;
		return n;
	};

	BufferOStream::int_type BufferOStream::overflow(int_type ch)
	{
		char c = (char)ch;
		return (int)xsputn(&c, 1);
	}

	BufferOStream::pos_type BufferOStream::seekpos(pos_type pos, std::ios_base::openmode which)
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	BufferOStream::pos_type BufferOStream::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
	{
		if ((which & ios_base::out) == ios_base::out)
		{
			switch (dir)
			{
			case std::ios_base::beg:
				writePos = off;
				break;
			case std::ios_base::cur:
				writePos += off;
				break;
			case std::ios_base::end:
				writePos = buffer.size() + off;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid seek direction");
			}
			return writePos;
		}
		return pos_type(off_type(-1));
	}
}
