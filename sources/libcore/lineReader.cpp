#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/lineReader.h>

namespace cage
{
	bool lineReaderBuffer::readLine(string &line)
	{
		if (size == 0)
			return false;

		uint32 len = 0;
		while (len < size && buffer[len] != '\n')
			len++;

		if (len >= string::MaxLength)
			CAGE_THROW_ERROR(exception, "line too long");

		line = string(buffer, len);

#ifdef CAGE_SYSTEM_WINDOWS
		if (!line.empty() && line[line.length() - 1] == '\r')
			line = line.subString(0, line.length() - 1);
#endif

		if (len == size)
		{
			size = 0;
			buffer = nullptr;
		}
		else
		{
			size -= len + 1;
			buffer += len + 1;
		}

		return true;
	}
}