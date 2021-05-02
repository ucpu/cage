#include <cage-core/typeIndex.h>
#include <cage-core/enumerate.h>

#include <vector>
#include <cstring>

namespace cage
{
	namespace
	{
		struct Type
		{
			const char *name = nullptr;
			uintPtr size = 0;
			uintPtr alignment = 0;
		};

		struct Data
		{
			std::vector<Type> values;

			Data()
			{
				values.reserve(100);
			}

			uint32 index(const char *name, uintPtr size, uintPtr alignment)
			{
				for (auto it : cage::enumerate(values))
					if (strcmp(it->name, name) == 0)
						return numeric_cast<uint32>(it.index);
				values.push_back({ name, size, alignment });
				return numeric_cast<uint32>(values.size() - 1);
			}
		};

		Data &data()
		{
			static Data d;
			return d;
		}
	}

	namespace privat
	{
		uint32 typeIndexInit(const char *name, uintPtr size, uintPtr alignment)
		{
			return data().index(name, size, alignment);
		}
	}

	namespace detail
	{
		uintPtr typeSize(uint32 index)
		{
			return data().values[index].size;
		}

		uintPtr typeAlignment(uint32 index)
		{
			return data().values[index].alignment;
		}
	}
}
