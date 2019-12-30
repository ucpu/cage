#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/guid.h>

#include <random>

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

		string guidToString(const uint8 *data, uint32 size)
		{
			string res;
			for (uint32 i = 0; i < size; i++)
			{
				char a = (char)(data[i] / 16) + (data[i] / 16 < 10 ? '0' : 'a' - 10);
				res += string(&a, 1);
				char b = (char)(data[i] % 16) + (data[i] % 16 < 10 ? '0' : 'a' - 10);
				res += string(&b, 1);
			}
			return res;
		}
	}
}
