#include <cage-core/typeIndex.h>
#include <cage-core/concurrent.h>

#include <array>
#include <cstring>
#include <exception> // std::terminate

namespace cage
{
	namespace
	{
		struct Type
		{
			StringLiteral name;
			uintPtr size = 0;
			uintPtr alignment = 0;
		};

		struct Data
		{
			Holder<Mutex> mut = newMutex();
			std::array<Type, 1000> values = {};
			uint32 cnt = 0;

			uint32 index(StringLiteral name, uintPtr size, uintPtr alignment)
			{
				ScopeLock lock(mut);
				for (uint32 i = 0; i < cnt; i++)
					if (strcmp(values[i].name, name) == 0)
						return i;
				if (cnt == values.size())
					CAGE_THROW_CRITICAL(Exception, "too many types in typeIndex");
				Type t;
				t.name = name;
				t.size = size;
				t.alignment = alignment;
				values[cnt] = t;
				return cnt++;
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
		uint32 typeIndexInitImpl(StringLiteral name, uintPtr size, uintPtr alignment) noexcept
		{
			return data().index(name, size, alignment);
		}
	}

	namespace detail
	{
		uintPtr typeSize(uint32 index) noexcept
		{
			return data().values[index].size;
		}

		uintPtr typeAlignment(uint32 index) noexcept
		{
			return data().values[index].alignment;
		}
	}
}
