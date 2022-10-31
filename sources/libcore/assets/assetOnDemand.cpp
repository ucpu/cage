#include <cage-core/concurrent.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>

#include <robin_hood.h>
#include <algorithm>

namespace cage
{
	namespace
	{
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
					if (it->second != tick)
					{
						assets->remove(it->first);
						it = lastUse.erase(it);
					}
					else
						it++;
				}
				tick++;
			}
		};
	}

	void AssetOnDemand::process()
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		impl->process();
	}

	Holder<void> AssetOnDemand::get_(uint32 scheme, uint32 assetName, bool throwOnInvalidScheme, bool autoLoad)
	{
		AssetOnDemandImpl *impl = (AssetOnDemandImpl *)this;
		{
			auto r = impl->assets->get_(scheme, assetName, throwOnInvalidScheme);
			ScopeLock lock(impl->mut, ReadLockTag());
			auto it = impl->lastUse.find(assetName);
			if (it != impl->lastUse.end())
				it->second = impl->tick;
			if (r)
				return r;
		}
		if (autoLoad)
		{
			ScopeLock lock(impl->mut, WriteLockTag());
			if (impl->lastUse.count(assetName) == 0)
			{
				impl->lastUse[assetName] = impl->tick;
				impl->assets->add(assetName);
			}
		}
		return {};
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
