#include <algorithm>

#include <unordered_dense.h>

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/concurrent.h>

namespace cage
{
	namespace
	{
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
				ScopeLock lock(mut, WriteLockTag());
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

			void update(uint32 assetId, bool autoLoad)
			{
				{
					ScopeLock lock(mut, ReadLockTag()); // read lock is sufficient: we update the value _inside_ the map, not the map itself
					auto it = lastUse.find(assetId);
					if (it != lastUse.end())
					{
						it->second = tick;
						return;
					}
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

	void AssetsOnDemand::preload(uint32 assetId)
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		impl->update(assetId, true);
	}

	void AssetsOnDemand::preload(PointerRange<const uint32> assetsIds)
	{
		//AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		// todo optimize
		for (uint32 id : assetsIds)
			preload(id);
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

	AssetsManager *AssetsOnDemand::assetsManager() const
	{
		const AssetOnDemandImpl *impl = (const AssetOnDemandImpl *)this;
		return impl->assets;
	}

	Holder<void> AssetsOnDemand::get1_(uint32 scheme, uint32 assetId, bool autoLoad)
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		auto r = impl->assets->get1_(scheme, assetId);
		impl->update(assetId, autoLoad && !r);
		return r;
	}

	Holder<void> AssetsOnDemand::get2_(uint32 typeHash, uint32 assetId, bool autoLoad)
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		auto r = impl->assets->get2_(typeHash, assetId);
		impl->update(assetId, autoLoad && !r);
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
