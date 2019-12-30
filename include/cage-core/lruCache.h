
#include <vector>
#include <unordered_map>

#include <cage-core/enumerate.h>

namespace cage
{
	template<class Key, class Value, class Hasher = std::hash<Key>>
	struct LruCache : private Immovable
	{
	private:
		struct Data
		{
			Key key;
			Value value;
			uint32 p, n;
			bool valid;
			Data() : key(), value(), p(m), n(m), valid(false) {}
		};

		std::vector<Data> data;
		std::unordered_map<Key, uint32, Hasher> indices;
		uint32 head;
		const uint32 capacity;

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
		explicit LruCache(uint32 capacity) : head(0), capacity(capacity)
		{
			CAGE_ASSERT(capacity >= 3);
			clear();
		}

		const Value *find(const Key &k)
		{
			auto it = indices.find(k);
			if (it == indices.end())
				return nullptr;
			Data &d = data[it->second];
			CAGE_ASSERT(d.valid);
			CAGE_ASSERT(d.key == k);
			if (it->second == head)
				head = d.n;
			else
				move(it->second, head);
			return &d.value;
		}

		const Value *set(const Key &k, const Value &value)
		{
			CAGE_ASSERT(indices.count(k) == 0);
			Data &h = data[head];
			if (h.valid)
				indices.erase(h.key);
			indices[k] = head;
			head = h.n;
			h.key = k;
			h.value = value;
			h.valid = true;
			return &h.value;
		}

		const Value *set(const Key &k, Value &&value)
		{
			CAGE_ASSERT(indices.count(k) == 0);
			Data &h = data[head];
			if (h.valid)
				indices.erase(h.key);
			indices[k] = head;
			head = h.n;
			h.key = k;
			h.value = templates::move(value);
			h.valid = true;
			return &h.value;
		}

		void erase(const Key &k)
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
}