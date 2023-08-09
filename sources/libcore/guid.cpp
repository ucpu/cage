#include <random>

#include <cage-core/guid.h>
#include <cage-core/math.h>

namespace cage
{
	namespace privat
	{
		void generateRandomData(uint8 *target, uint32 size)
		{
			std::random_device rd;
			while (size)
			{
				const std::random_device::result_type tmp = rd();
				const uint32 n = min(numeric_cast<uint32>(sizeof(tmp)), size);
				detail::memcpy(target, &tmp, n);
				target += n;
				size -= n;
			}
		}

		String guidToString(const uint8 *data, uint32 size)
		{
			String res;
			for (uint32 i = 0; i < size; i++)
			{
				char a = (char)(data[i] / 16) + (data[i] / 16 < 10 ? '0' : 'a' - 10);
				res += String({ &a, &a + 1 });
				char b = (char)(data[i] % 16) + (data[i] % 16 < 10 ? '0' : 'a' - 10);
				res += String({ &b, &b + 1 });
			}
			return res;
		}
	}
}
