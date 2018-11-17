
#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	serializer &serializer::write(const void *data, uintPtr size)
	{
		uintPtr o = buffer.size();
		buffer.resizeGrowSmart(buffer.size() + size);
		detail::memcpy(buffer.data() + o, data, size);
		return *this;
	}

	char *serializer::accessRaw(uintPtr size)
	{
		if (size == 0)
			return buffer.data();
		uintPtr o = buffer.size();
		buffer.resizeGrowSmart(buffer.size() + size);
		return buffer.data() + o;
	}

	deserializer::deserializer(const memoryBuffer &buffer) : current(buffer.data()), end(current + buffer.size())
	{}
}
