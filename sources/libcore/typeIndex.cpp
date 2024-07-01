#include <array>
#include <cstring>

#include <unordered_dense.h>

#include <cage-core/concurrent.h>
#include <cage-core/typeIndex.h>

namespace cage
{
	namespace
	{
		struct Type
		{
			PointerRange<const char> name;
			uintPtr size = 0;
			uintPtr alignment = 0;
			uint32 hash = 0;
		};

		struct Data
		{
			Holder<Mutex> mut = newMutex();
			ankerl::unordered_dense::map<uint32, uint32> hashToIndex;
			std::array<Type, 10000> values = {};
			uint32 cnt = 0;

			uint32 index(PointerRange<const char> name, uintPtr size, uintPtr alignment)
			{
				ScopeLock lock(mut);
				for (uint32 i = 0; i < cnt; i++)
					if (strcmp(values[i].name.data(), name.data()) == 0)
						return i;
				if (cnt == values.size())
					CAGE_THROW_CRITICAL(Exception, "too many types in typeIndex");
				Type t;
				t.name = name;
				t.size = size;
				t.alignment = alignment;
				t.hash = hashBuffer(name);
				if (hashToIndex.count(t.hash))
					CAGE_THROW_CRITICAL(Exception, "hash collision in typeHash");
				values[cnt] = t;
				hashToIndex[t.hash] = cnt;
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
		uint32 typeIndexInitImpl(PointerRange<const char> name, uintPtr size, uintPtr alignment)
		{
			return data().index(name, size, alignment);
		}
	}

	namespace detail
	{
		uintPtr typeSizeByIndex(uint32 index)
		{
			return data().values[index].size;
		}

		uintPtr typeAlignmentByIndex(uint32 index)
		{
			return data().values[index].alignment;
		}

		uint32 typeHashByIndex(uint32 index)
		{
			const uint32 res = data().values[index].hash;
			CAGE_ASSERT(data().hashToIndex.find(res)->second == index);
			return res;
		}

		uint32 typeIndexByHash(uint32 hash)
		{
			const auto it = data().hashToIndex.find(hash);
			CAGE_ASSERT(it != data().hashToIndex.end());
			const uint32 res = it->second;
			CAGE_ASSERT(data().values[res].hash == hash);
			return res;
		}
	}
}
