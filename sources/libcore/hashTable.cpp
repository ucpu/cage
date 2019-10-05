#include <robin_hood.h>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/hashTable.h>

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
			class hashTableImpl : public hashTablePriv
			{
			public:
				hashTableImpl(const hashTableCreateConfig &config)
				{
					data.reserve(config.initItems);
				}

				robin_hood::unordered_map<uint32, void*> data;
			};
		}

		typedef robin_hood::unordered_map<uint32, void*>::iterator iterator;
		CAGE_ASSERT_COMPILE(sizeof(hashTableItPriv) >= sizeof(iterator), hashTableItPriv_must_be_at_least_as_large_st_the_original_iterator);

		const hashTablePairPriv &hashTableItPriv::operator *() const
		{
			auto &it = ((iterator*)this)->operator*();
			return *(hashTablePairPriv*)&it;
		}

		const hashTablePairPriv *hashTableItPriv::operator ->() const
		{
			return (hashTablePairPriv*)(*(iterator*)this).operator->();
		}

		void hashTableItPriv::operator ++()
		{
			++(*(iterator*)this);
		}

		bool hashTableItPriv::operator == (const hashTableItPriv &other) const
		{
			return (*(iterator*)this) == (*(iterator*)&other);
		}

		void hashTablePriv::add(uint32 name, void *value)
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			auto r = impl->data.emplace(name, value);
			if (!r.second)
				CAGE_THROW_ERROR(exception, "duplicate key");
		}

		void hashTablePriv::remove(uint32 name)
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			impl->data.erase(name);
		}

		void *hashTablePriv::tryGet(uint32 name) const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			auto it = impl->data.find(name);
			if (it == impl->data.end())
				return nullptr;
			return it->second;
		}

		void *hashTablePriv::get(uint32 name) const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			try
			{
				return impl->data.at(name);
			}
			catch (std::out_of_range&)
			{
				CAGE_THROW_ERROR(exception, "hash table item not found");
			}
			return nullptr;
		}

		bool hashTablePriv::exists(uint32 name) const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			return impl->data.count(name) == 1;
		}

		hashTableItPriv hashTablePriv::begin() const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			auto it = impl->data.begin();
			return *(hashTableItPriv*)&it;
		}

		hashTableItPriv hashTablePriv::end() const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			auto it = impl->data.end();
			return *(hashTableItPriv*)&it;
		}

		uint32 hashTablePriv::count() const
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			return numeric_cast<uint32>(impl->data.size());
		}

		void hashTablePriv::clear()
		{
			hashTableImpl *impl = (hashTableImpl*)this;
			impl->data.clear();
		}

		holder<hashTablePriv> newHashTable(const hashTableCreateConfig &config)
		{
			return detail::systemArena().createImpl<hashTablePriv, hashTableImpl>(config);
		}
	}

	hashTableCreateConfig::hashTableCreateConfig() : initItems(0)
	{}
}
