#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	namespace
	{
		class lineReaderImpl : public lineReader
		{
		public:
			lineReaderImpl(const char *buff, uintPtr size) : buffer(buff), size(size)
			{}

			lineReaderImpl(memoryBuffer &&buff) : mb(templates::move(buff)), buffer(mb.data()), size(mb.size())
			{}

			memoryBuffer mb;
			const char *buffer;
			uintPtr size;
		};
	}

	bool lineReader::readLine(string &line)
	{
		lineReaderImpl *impl = (lineReaderImpl*)this;

		if (impl->size == 0)
			return false;

		uint32 len = 0;
		while (len < impl->size && impl->buffer[len] != '\n')
			len++;

		line = string(impl->buffer, len);

		if (!line.empty() && line[line.length() - 1] == '\r')
			line = line.subString(0, line.length() - 1);

		if (len == impl->size)
		{
			impl->size = 0;
			impl->buffer = nullptr;
		}
		else
		{
			impl->size -= len + 1;
			impl->buffer += len + 1;
		}

		return true;
	}

	uintPtr lineReader::left() const
	{
		lineReaderImpl *impl = (lineReaderImpl*)this;
		return impl->size;
	}

	holder<lineReader> newLineReader(const char *buffer, uintPtr size)
	{
		return detail::systemArena().createImpl<lineReader, lineReaderImpl>(buffer, size);
	}

	holder<lineReader> newLineReader(const memoryBuffer &buffer)
	{
		return detail::systemArena().createImpl<lineReader, lineReaderImpl>(buffer.data(), buffer.size());
	}

	holder<lineReader> newLineReader(memoryBuffer &&buffer)
	{
		return detail::systemArena().createImpl<lineReader, lineReaderImpl>(templates::move(buffer));
	}
}
