
#include <vector>
#include <unordered_map>

#include <cage-core/enumerate.h>

namespace cage
{
	template<class Key, class Value, uint32 Capacity, class Hasher = std::hash<Key>>
	struct lruCache
	{
	private:
		static_assert(Capacity >= 3, "minimum capacity");

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

	public:
		lruCache() : head(0)
		{
			purge();
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
			{
				Data &h = data[head];
				data[d.p].n = d.n;
				data[d.n].p = d.p;
				d.p = h.p;
				d.n = head;
				data[h.p].n = it->second;
				h.p = it->second;
			}
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

		void purge()
		{
			indices.clear();
			indices.reserve(Capacity);
			data.clear();
			data.resize(Capacity);
			{
				Data &d = data[0];
				d.p = Capacity - 1;
				d.n = 1;
			}
			for (uint32 i = 1; i < Capacity - 1; i++)
			{
				Data &d = data[i];
				d.p = i - 1;
				d.n = i + 1;
			}
			{
				Data &d = data[Capacity - 1];
				d.p = Capacity - 2;
				d.n = 0;
			}
			head = 0;
		}
	};
}
