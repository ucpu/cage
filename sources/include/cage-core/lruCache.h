#ifndef guard_lruCache_h_ABEDFC7ADD4A
#define guard_lruCache_h_ABEDFC7ADD4A

#include "core.h"

#include <robin_hood.h>

#include <vector>
#include <optional>

namespace cage
{
	template<class Key, class Value, class Hasher = std::hash<Key>>
	struct LruCache : private Immovable
	{
	private:
		struct Data
		{
			Key key = Key();
			Value value = Value();
			uint32 p = m, n = m;
			bool valid = false;
		};

		std::vector<Data> data;
		robin_hood::unordered_map<Key, uint32, Hasher> indices;
		uint32 head = 0;
		const uint32 capacity = 0;

		void move(uint32 which, uint32 where) // moves _which_ before the _where_
		{
			Data &d = data[which];
			Data &h = data[where];
			data[d.p].n = d.n;
			data[d.n].p = d.p;
			d.p = h.p;
			d.n = where;
			data[h.p].n = which;
			h.p = which;
		}

	public:
		explicit LruCache(uint32 capacity) : capacity(capacity)
		{
			CAGE_ASSERT(capacity >= 3);
			clear();
		}

		std::optional<Value> find(Key k)
		{
			auto it = indices.find(k);
			if (it == indices.end())
				return {};
			Data &d = data[it->second];
			CAGE_ASSERT(d.valid);
			CAGE_ASSERT(d.key == k);
			if (it->second == head)
				head = d.n;
			else
				move(it->second, head);
			return d.value;
		}

		Value set(Key k, Value value)
		{
			CAGE_ASSERT(indices.count(k) == 0);
			Data &h = data[head];
			if (h.valid)
				indices.erase(h.key);
			indices[k] = head;
			head = h.n;
			h.key = k;
			h.value = std::move(value);
			h.valid = true;
			return h.value;
		}

		void erase(Key k)
		{
			auto it = indices.find(k);
			if (it == indices.end())
				return;
			Data &d = data[it->second];
			CAGE_ASSERT(d.valid);
			d.key = Key();
			d.value = Value();
			d.valid = false;
			if (it->second != head)
			{
				move(it->second, head);
				head = it->second;
			}
			indices.erase(it);
		}

		void clear()
		{
			indices.clear();
			indices.reserve(capacity);
			data.clear();
			data.resize(capacity);
			{
				Data &d = data[0];
				d.p = capacity - 1;
				d.n = 1;
			}
			for (uint32 i = 1; i < capacity - 1; i++)
			{
				Data &d = data[i];
				d.p = i - 1;
				d.n = i + 1;
			}
			{
				Data &d = data[capacity - 1];
				d.p = capacity - 2;
				d.n = 0;
			}
			head = 0;
		}
	};

	template<class Key, class Value, class Hasher>
	struct LruCache<Key, Holder<Value>, Hasher> : private Immovable
	{
	private:
		struct Data
		{
			Holder<Value> data;

			Data() = default;
			Data(Data &&other) = default;
			Data(const Data &other) : data(other.data.share()) {}
			Data &operator = (Data &&other) = default;
			Data &operator = (const Data &other) { if (&other != this) { data = other.data.share(); } return *this; }
		};

		LruCache<Key, Data, Hasher> cache;

	public:
		explicit LruCache(uint32 capacity) : cache(capacity)
		{}

		Holder<Value> find(Key k)
		{
			auto v = cache.find(k);
			if (v)
				return std::move(v->data);
			return {};
		}

		Holder<Value> set(Key k, Holder<Value> value)
		{
			Data d;
			d.data = std::move(value);
			return cache.set(k, d).data;
		}

		void erase(Key k)
		{
			cache.erase(k);
		}

		void clear()
		{
			cache.clear();
		}
	};
}

#endif // guard_lruCache_h_ABEDFC7ADD4A
