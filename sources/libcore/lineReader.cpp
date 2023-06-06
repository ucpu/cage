#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

namespace cage
{
	namespace detail
	{
		uintPtr readLine(PointerRange<const char> &output, PointerRange<const char> input, bool streaming)
		{
			const uintPtr size = input.size();
			const char *buffer = input.data();

			if (size == 0)
				return 0;

			uintPtr len = 0;
			while (len < size && buffer[len] != '\n')
				len++;

			if (streaming && len == size)
				return 0;

			output = { buffer, buffer + len };

			if (len && output[output.size() - 1] == '\r')
				output = { buffer, buffer + len - 1 };

			return len + (len < size); // plus the new line
		}

		uintPtr readLine(String &output, PointerRange<const char> input, bool streaming)
		{
			PointerRange<const char> tmp;
			uintPtr res = readLine(tmp, input, streaming);
			output = String(tmp); // may throw
			return res;
		}
	}

	namespace
	{
		class LineReaderImpl : public LineReader
		{
		public:
			LineReaderImpl(const char *buff, uintPtr size) : buffer(buff), size(size) {}

			LineReaderImpl(MemoryBuffer &&buff) : mb(std::move(buff)), buffer(mb.data()), size(mb.size()) {}

			MemoryBuffer mb;
			const char *buffer = nullptr;
			uintPtr size = 0;
		};
	}

	bool LineReader::readLine(PointerRange<const char> &line)
	{
		LineReaderImpl *impl = (LineReaderImpl *)this;
		uintPtr l = detail::readLine(line, { impl->buffer, impl->buffer + impl->size }, false);
		impl->buffer += l;
		impl->size -= l;
		return !!l;
	}

	bool LineReader::readLine(String &line)
	{
		LineReaderImpl *impl = (LineReaderImpl *)this;
		uintPtr l = detail::readLine(line, { impl->buffer, impl->buffer + impl->size }, false);
		impl->buffer += l;
		impl->size -= l;
		return !!l;
	}

	uintPtr LineReader::remaining() const
	{
		LineReaderImpl *impl = (LineReaderImpl *)this;
		return impl->size;
	}

	Holder<LineReader> newLineReader(PointerRange<const char> buffer)
	{
		return systemMemory().createImpl<LineReader, LineReaderImpl>(buffer.data(), buffer.size());
	}

	Holder<LineReader> newLineReader(MemoryBuffer &&buffer)
	{
		return systemMemory().createImpl<LineReader, LineReaderImpl>(std::move(buffer));
	}
}
