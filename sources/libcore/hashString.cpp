#include <cage-core/hashString.h>
#include <cage-core/memoryBuffer.h>

#include <cstring>

namespace cage
{
	namespace detail
	{
		uint32 hashBuffer(const char *buffer, uintPtr size) noexcept
		{
			uint32 hash = HashCompile::Offset;
			while (size--)
			{
				hash ^= *buffer++;
				hash *= HashCompile::Prime;
			}
			return hash;
		}

		uint32 hashBuffer(const MemoryBuffer &buffer) noexcept
		{
			return hashBuffer(buffer.data(), buffer.size());
		}

		uint32 hashString(const char *str) noexcept
		{
			return hashBuffer(str, std::strlen(str));
		}
	}
}
