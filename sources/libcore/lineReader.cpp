#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	namespace detail
	{
		bool readLine(string &output, const char *&buffer, uintPtr &size, bool lfOnly)
		{
			if (size == 0)
				return false;

			uintPtr len = 0;
			while (len < size && buffer[len] != '\n')
				len++;

			if (lfOnly && len == size)
				return false;

			if (len > detail::numeric_limits<uint32>::max())
				CAGE_THROW_ERROR(Exception, "line too long");

			output = string(buffer, numeric_cast<uint32>(len));

			if (!output.empty() && output[output.length() - 1] == '\r')
				output = output.subString(0, output.length() - 1);

			if (len == size)
				size = 0;
			else
				size -= len + 1;

			buffer += len + 1;
			return true;
		}
	}

	namespace
	{
		class lineReaderImpl : public LineReader
		{
		public:
			lineReaderImpl(const char *buff, uintPtr size) : buffer(buff), size(size)
			{}

			lineReaderImpl(MemoryBuffer &&buff) : mb(templates::move(buff)), buffer(mb.data()), size(mb.size())
			{}

			MemoryBuffer mb;
			const char *buffer;
			uintPtr size;
		};
	}

	bool LineReader::readLine(string &line)
	{
		lineReaderImpl *impl = (lineReaderImpl*)this;
		return detail::readLine(line, impl->buffer, impl->size, false);
	}

	uintPtr LineReader::left() const
	{
		lineReaderImpl *impl = (lineReaderImpl*)this;
		return impl->size;
	}

	Holder<LineReader> newLineReader(const char *buffer, uintPtr size)
	{
		return detail::systemArena().createImpl<LineReader, lineReaderImpl>(buffer, size);
	}

	Holder<LineReader> newLineReader(const MemoryBuffer &buffer)
	{
		return detail::systemArena().createImpl<LineReader, lineReaderImpl>(buffer.data(), buffer.size());
	}

	Holder<LineReader> newLineReader(MemoryBuffer &&buffer)
	{
		return detail::systemArena().createImpl<LineReader, lineReaderImpl>(templates::move(buffer));
	}
}
