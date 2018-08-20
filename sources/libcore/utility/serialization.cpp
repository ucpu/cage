
#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/serialization.h>

namespace cage
{
	serializer &serializer::write(const void *data, uintPtr size)
	{
		uintPtr o = buffer.size();
		buffer.resizeGrowSmart(buffer.size() + size);
		detail::memcpy(buffer.data() + o, data, size);
		return *this;
	}

	char *serializer::access(uintPtr size)
	{
		uintPtr o = buffer.size();
		buffer.resizeGrowSmart(buffer.size() + size);
		return buffer.data() + o;
	}

	deserializer::deserializer(const memoryBuffer &buffer) : current(buffer.data()), end(current + buffer.size())
	{}
}
