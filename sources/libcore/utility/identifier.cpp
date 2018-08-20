#include <random>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/utility/identifier.h>

namespace cage
{
	namespace privat
	{
		void generateRandomData(uint8 *target, uint32 size)
		{
			std::random_device rd;
			while (size--)
				*target++ = (uint8)rd();
		}
	}
}
