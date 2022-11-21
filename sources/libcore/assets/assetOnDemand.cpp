#include <cage-core/concurrent.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>

#include <robin_hood.h>
#include <algorithm>

namespace cage
{
	namespace
	{
		// difference that considers overflows
		constexpr uint32 diff(uint32 a, uint32 b)
		{
			return b - a;
		}
		static_assert(diff(13, 13) == 0);
		static_assert(diff(13, 15) == 2);
		static_assert(diff(4294967295, 4294967295) == 0);
		static_assert(diff(4294967295, 0) == 1);
		static_assert(diff(4294967290, 5) == 11);

		class AssetOnDemandImpl : public AssetOnDemand
		{
		public:
			AssetManager *assets = nullptr;
			Holder<RwMutex> mut = newRwMutex();
			robin_hood::unordered_map<uint32, uint32> lastUse;
			uint32 tick = 0;

			AssetOnDemandImpl(AssetManager *assets) : assets(assets)
			{}

			~AssetOnDemandImpl()
			{
				for (const auto &it : lastUse)
					assets->remove(it.first);
			}

			void process()
			{
				ScopeLock lock(mut, WriteLockTag());
				auto it = lastUse.begin();
				while (it != lastUse.end())
				{
					if (tick - it->second > 20)
					{
						assets->remove(it->first);
						it = lastUse.erase(it);
					}
					else
						it++;
				}
				tick++;
			}

			void clear()
			{
				ScopeLock lock(mut, WriteLockTag());
				for (const auto &it : lastUse)
					assets->remove(it.first);
				lastUse.clear();
				tick++;
			}

			void update(bool valid, uint32 assetName, bool autoLoad)
			{
				{
					ScopeLock lock(mut, ReadLockTag()); // read lock is sufficient: we update the value _inside_ the map, not the map itself
					auto it = lastUse.find(assetName);
					if (it != lastUse.end())
						it->second = tick;
					if (valid)
						return;
				}
				if (autoLoad)
				{
					ScopeLock lock(mut, WriteLockTag());
					if (lastUse.count(assetName) == 0) // check again after reacquiring the lock
					{
						lastUse[assetName] = tick;
						assets->add(assetName);
					}
				}
			}
		};
	}

	void AssetOnDemand::process()
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		impl->process();
	}

	void AssetOnDemand::clear()
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		impl->clear();
	}

	Holder<void> AssetOnDemand::get_(uint32 scheme, uint32 assetName, bool throwOnInvalidScheme, bool autoLoad)
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		auto r = impl->assets->get_(scheme, assetName, throwOnInvalidScheme);
		impl->update(!!r, assetName, autoLoad);
		return r;
	}

	uint32 AssetOnDemand::schemeTypeHash_(uint32 scheme) const
	{
		const AssetOnDemandImpl *impl = (const AssetOnDemandImpl *)this;
		return impl->assets->schemeTypeHash_(scheme);
	}

	Holder<AssetOnDemand> newAssetOnDemand(AssetManager *assets)
	{
		return systemMemory().createImpl<AssetOnDemand, AssetOnDemandImpl>(assets);
	}
}
