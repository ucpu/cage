#include <chrono>
#include <random>

#include <cage-core/concurrent.h>
#include <cage-core/guid.h>
#include <cage-core/math.h>
#include <cage-core/systemInformation.h>

namespace cage
{
	namespace privat
	{
		void generateRandomData(uint8 *const target, const uint32 size)
		{
			if (size == 0)
				return;

			{ // initial entropy by random device
				std::random_device rd;
				uint8 *a = target;
				uint8 *b = target + size;
				while (a != b)
				{
					const std::random_device::result_type tmp = rd();
					const uint32 n = min(numeric_cast<uint32>(sizeof(tmp)), size);
					detail::memcpy(a, &tmp, n);
					a += n;
				}
			}

			{ // mix with additional sources of enthropy
				uint32 pos = 0;
				const auto &mix = [&](auto val)
				{
					for (uint32 i = 0; i < sizeof(val); i++)
					{
						const uint8 bt = (uint8)(val >> (i * 8)); // i-th byte
						target[(pos++) % size] ^= bt;
					}
				};
				mix(currentThreadId());
				mix(currentProcessId());
				const uint64 time = uint64(std::chrono::high_resolution_clock::now().time_since_epoch().count());
				mix(time);
				const uintPtr addrVar = (uintPtr)(void *)(&size); // address of local variable
				mix(addrVar);
				const uintPtr addrFnc = (uintPtr)(void *)(&generateRandomData); // address of a function
				mix(addrFnc);
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
