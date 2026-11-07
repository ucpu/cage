#include <array>
#include <cstring>

#include <unordered_dense.h>

#include <cage-core/concurrent.h>
#include <cage-core/typeIndex.h>

namespace std
{
	template<>
	struct hash<cage::TypeHash>
	{
		auto operator()(const cage::TypeHash &id) const noexcept { return hash<decltype(id.value)>()(id.value); }
	};
}

namespace cage
{
	namespace
	{
		struct Type
		{
			PointerRange<const char> name;
			uintPtr size = 0;
			uintPtr alignment = 0;
			TypeHash hash = {};
		};

		struct Data
		{
			Holder<Mutex> mut = newMutex();
			ankerl::unordered_dense::map<TypeHash, TypeIndex> hashToIndex;
			std::array<Type, 10'000> values = {};
			uint32 cnt = 0;

			TypeIndex index(PointerRange<const char> name, uintPtr size, uintPtr alignment)
			{
				ScopeLock lock(mut);
				for (uint32 i = 0; i < cnt; i++)
					if (strcmp(values[i].name.data(), name.data()) == 0)
						return TypeIndex(i);
				if (cnt == values.size())
					CAGE_THROW_CRITICAL(Exception, "too many types in typeIndex");
				Type t;
				t.name = name;
				t.size = size;
				t.alignment = alignment;
				t.hash = TypeHash(hashBuffer(name));
				if (hashToIndex.count(t.hash))
					CAGE_THROW_CRITICAL(Exception, "hash collision in typeIndex");
				values[cnt] = t;
				hashToIndex[t.hash] = TypeIndex(cnt);
				return TypeIndex(cnt++);
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
		TypeIndex typeIndexInitImpl(PointerRange<const char> name, uintPtr size, uintPtr alignment)
		{
			return data().index(name, size, alignment);
		}
	}

	namespace detail
	{
		uintPtr typeSizeByIndex(TypeIndex index)
		{
			return data().values[index.value].size;
		}

		uintPtr typeAlignmentByIndex(TypeIndex index)
		{
			return data().values[index.value].alignment;
		}

		TypeHash typeHashByIndex(TypeIndex index)
		{
			const TypeHash res = data().values[index.value].hash;
			CAGE_ASSERT(data().hashToIndex.find(res)->second == index);
			return res;
		}

		TypeIndex typeIndexByHash(TypeHash hash)
		{
			const auto it = data().hashToIndex.find(hash);
			CAGE_ASSERT(it != data().hashToIndex.end());
			const TypeIndex res = it->second;
			CAGE_ASSERT(data().values[res.value].hash == hash);
			return res;
		}
	}
}
