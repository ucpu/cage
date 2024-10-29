#include <algorithm>

#include <unordered_dense.h>

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/concurrent.h>

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

		class AssetOnDemandImpl : public AssetsOnDemand
		{
		public:
			AssetsManager *assets = nullptr;
			Holder<RwMutex> mut = newRwMutex();
			ankerl::unordered_dense::map<uint32, uint32> lastUse;
			uint32 tick = 0;

			AssetOnDemandImpl(AssetsManager *assets) : assets(assets) {}

			~AssetOnDemandImpl()
			{
				for (const auto &it : lastUse)
					assets->unload(it.first);
			}

			void process()
			{
				ScopeLock lock(mut, WriteLockTag());
				auto it = lastUse.begin();
				while (it != lastUse.end())
				{
					if (tick - it->second > 20)
					{
						assets->unload(it->first);
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
					assets->unload(it.first);
				lastUse.clear();
				tick++;
			}

			void update(bool valid, uint32 assetId, bool autoLoad)
			{
				{
					ScopeLock lock(mut, ReadLockTag()); // read lock is sufficient: we update the value _inside_ the map, not the map itself
					auto it = lastUse.find(assetId);
					if (it != lastUse.end())
						it->second = tick;
					if (valid)
						return;
				}
				if (autoLoad)
				{
					ScopeLock lock(mut, WriteLockTag());
					if (lastUse.count(assetId) == 0) // check again after reacquiring the lock
					{
						lastUse[assetId] = tick;
						assets->load(assetId);
					}
				}
			}
		};
	}

	void AssetsOnDemand::process()
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		impl->process();
	}

	void AssetsOnDemand::clear()
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		impl->clear();
	}

	Holder<void> AssetsOnDemand::get_(uint32 scheme, uint32 assetId, bool autoLoad)
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		auto r = impl->assets->get_(scheme, assetId);
		impl->update(!!r, assetId, autoLoad);
		return r;
	}

	uint32 AssetsOnDemand::schemeTypeHash_(uint32 scheme) const
	{
		const AssetOnDemandImpl *impl = (const AssetOnDemandImpl *)this;
		return impl->assets->schemeTypeHash_(scheme);
	}

	Holder<AssetsOnDemand> newAssetsOnDemand(AssetsManager *assets)
	{
		return systemMemory().createImpl<AssetsOnDemand, AssetOnDemandImpl>(assets);
	}
}
