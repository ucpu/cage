#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/utility/hashTable.h>
#include <cage-core/math.h>

namespace cage
{
	namespace detail
	{
		uint32 hash(uint32 key)
		{ // integer finalizer hash function
			key ^= key >> 16;
			key *= 0x85ebca6b;
			key ^= key >> 13;
			key *= 0xc2b2ae35;
			key ^= key >> 16;
			return key;
		}
	}

	namespace privat
	{
		namespace
		{
			using detail::hash;

			class hashTableImpl : public hashTablePriv
			{
			public:
				hashTableImpl(uint32 initItems, uint32 maxItems, float maxFillRate) : lines(nullptr), used(0), tombs(0), total(0), maxFillRate(maxFillRate), maxPages(0)
				{
					CAGE_ASSERT_RUNTIME(initItems <= maxItems);
					CAGE_ASSERT_RUNTIME(maxFillRate > 0 && maxFillRate < 1, maxFillRate);
					mem = newVirtualMemory();
					lines = (hashTableLineStruct*)mem->reserve(maxPages = numeric_cast<uint32>(detail::roundUpToMemoryPage(maxItems * sizeof(hashTableLineStruct)) / detail::memoryPageSize()));
					mem->increase(detail::roundUpToMemoryPage(initItems * sizeof(hashTableLineStruct)) / detail::memoryPageSize());
					total = capacity();
					clear();
				}

				void rehash()
				{
					if (used + 1 > total * maxFillRate)
					{
						if (mem->pages() == maxPages)
							CAGE_THROW_ERROR(outOfMemoryException, "hash table is out of memory", maxPages * detail::memoryPageSize());
						mem->increase(min(maxPages, numeric_cast<uint32>(mem->pages() * 1.5 + 1)) - mem->pages());
					}
					std::vector<hashTableLineStruct> tmp;
					tmp.reserve(used);
					for (hashTableLineStruct *i = lines, *e = lines + total; i != e; i++)
					{
						if (i->first)
							tmp.push_back(*i);
					}
					clear();
					total = capacity();
					for (auto i : tmp)
						add(i.first, i.second);
				}

				const uint32 capacity() const
				{
					return numeric_cast<uint32>(mem->pages() * detail::memoryPageSize() / sizeof(hashTableLineStruct));
				}

				hashTableLineStruct *lines;
				uint32 used;
				uint32 tombs;
				uint32 total;
				float maxFillRate;
				uint32 maxPages;
				holder<virtualMemoryClass> mem;
			};
		}

		bool hashTablePriv::exists(uint32 name) const
		{
			return get(name, true) != nullptr;
		}

		void *hashTablePriv::get(uint32 name, bool allowNull) const
		{
			CAGE_ASSERT_RUNTIME(name > 0, name);
			hashTableImpl *impl = (hashTableImpl*)this;
			uint32 h = name;
			uint32 iter = 0;
			while (iter++ < 100)
			{
				h = hash(h);
				uint32 k = h % impl->total;
				hashTableLineStruct &l = impl->lines[k];
				if (l.first == name)
					return l.second;
				if (l.first != 0 || l.second != nullptr)
					continue;
				if (allowNull)
					return nullptr;
				CAGE_THROW_ERROR(exception, "not found");
			}
			CAGE_THROW_CRITICAL(exception, "infinite cycle");
		}

		void hashTablePriv::add(uint32 name, void *value)
		{
			CAGE_ASSERT_RUNTIME(name > 0);
			CAGE_ASSERT_RUNTIME(value != nullptr);
			hashTableImpl *impl = (hashTableImpl*)this;
			if (impl->used + impl->tombs + 1 > impl->total * impl->maxFillRate)
				impl->rehash();
			uint32 h = name;
			uint32 iter = 0;
			while (iter++ < 100)
			{
				h = hash(h);
				uint32 k = h % impl->total;
				hashTableLineStruct &l = impl->lines[k];
				if (l.first == name)
					CAGE_THROW_ERROR(exception, "duplicate key");
				if (l.first == 0)
				{
					if (l.second != 0)
						impl->tombs--;
					l.first = name;
					l.second = value;
					impl->used++;
					return;
				}
			}
			CAGE_THROW_CRITICAL(exception, "infinite cycle");
		}

		void hashTablePriv::remove(uint32 name)
		{
			CAGE_ASSERT_RUNTIME(name > 0, name);
			hashTableImpl *impl = (hashTableImpl*)this;
			uint32 h = name;
			uint32 iter = 0;
			while (iter++ < 100)
			{
				h = hash(h);
				uint32 k = h % impl->total;
				hashTableLineStruct &l = impl->lines[k];
				if (l.first == name)
				{
					l.first = 0;
					l.second = (void*)1; // tomb
					impl->tombs++;
					impl->used--;
					return;
				}
				if (l.first == 0 && l.second == 0)
					return; // item not found
			}
			CAGE_THROW_CRITICAL(exception, "infinite cycle");
		}

		const hashTableLineStruct *hashTablePriv::begin() const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			const hashTableLineStruct *res = impl->lines - 1;
			next(res);
			return res;
		}

		const hashTableLineStruct *hashTablePriv::end() const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			return impl->lines + impl->total;
		}

		void hashTablePriv::next(const hashTableLineStruct *&it) const
		{
			it++;
			const hashTableLineStruct *e = end();
			while (it != e && !it->first)
				it++;
		}

		void hashTablePriv::clear()
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			impl->tombs = impl->used = 0;
			detail::memset(impl->lines, 0, sizeof(hashTableLineStruct) * impl->total);
		}

		uint32 hashTablePriv::count() const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			return impl->used;
		}

		holder<hashTablePriv> newHashTable(uint32 initItems, uint32 maxItems, float maxFillRate)
		{
			return detail::systemArena().createImpl<hashTablePriv, hashTableImpl>(initItems, maxItems, maxFillRate);
		}
	}
}
