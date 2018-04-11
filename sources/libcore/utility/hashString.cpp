#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/utility/hashString.h>

namespace cage
{
	namespace privat
	{
		const uint32 hashStringFunction(const char *str)
		{
			uint32 hash = GCHL_HASH_OFFSET;
			while (*str != 0)
			{
				hash ^= *str++;
				hash *= GCHL_HASH_PRIME;
			}
			return hash;
		}
	}
}