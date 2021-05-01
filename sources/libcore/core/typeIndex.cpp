#include <cage-core/typeIndex.h>
#include <cage-core/enumerate.h>

#include <vector>
#include <cstring>

namespace cage
{
	namespace privat
	{
		uint32 typeIndexInit(const char *ptr)
		{
			struct Data
			{
				std::vector<const char *> values;

				Data()
				{
					values.reserve(100);
				}

				uint32 val(const char *ptr)
				{
					for (auto it : cage::enumerate(values))
						if (strcmp(*it, ptr) == 0)
							return numeric_cast<uint32>(it.index);
					values.push_back(ptr);
					return numeric_cast<uint32>(values.size() - 1);
				}
			};

			static Data data;
			return data.val(ptr);
		}
	}
}
