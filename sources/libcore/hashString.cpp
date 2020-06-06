#include <cage-core/hashString.h>
#include <cage-core/memoryBuffer.h>

#include <cstring>

namespace cage
{
	namespace detail
	{
		uint32 hashBuffer(PointerRange<const char> buffer) noexcept
		{
			const char *b = buffer.begin();
			const char *e = buffer.end();
			uint32 hash = HashCompile::Offset;
			while (b != e)
			{
				hash ^= *b++;
				hash *= HashCompile::Prime;
			}
			return hash;
		}

		uint32 hashString(const char *str) noexcept
		{
			return hashBuffer({ str, str + std::strlen(str) });
		}
	}
}
