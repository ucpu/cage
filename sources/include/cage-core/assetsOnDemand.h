#ifndef guard_assetsOnDemand_h_wesdxfc74j
#define guard_assetsOnDemand_h_wesdxfc74j

#include <cage-core/typeIndex.h>

namespace cage
{
	class AssetsManager;

	class CAGE_CORE_API AssetsOnDemand : private Immovable
	{
	public:
		// begin thread-safe methods

		// returns null if the asset is not yet loaded, failed to load, or has different scheme
		template<uint32 Scheme, class T>
		Holder<T> get(uint32 assetId, bool autoLoad = true)
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			return get1_(Scheme, assetId, autoLoad).template cast<T>();
		}

		// returns null if the asset is not yet loaded, failed to load, or has different type
		template<class T>
		Holder<T> get(uint32 assetId, bool autoLoad = true)
		{
			return get2_(detail::typeHash<T>(), assetId, autoLoad).template cast<T>();
		}

		void preload(uint32 assetId);
		void preload(PointerRange<const uint32> assetsIds);

		void process();
		void clear();

		AssetsManager *assetsManager() const;

		// end thread-safe methods

	private:
		Holder<void> get1_(uint32 scheme, uint32 assetId, bool autoLoad);
		Holder<void> get2_(uint32 typeHash, uint32 assetId, bool autoLoad);
		uint32 schemeTypeHash_(uint32 scheme) const;
	};

	CAGE_CORE_API Holder<AssetsOnDemand> newAssetsOnDemand(AssetsManager *assets);
}

#endif // guard_assetsOnDemand_h_wesdxfc74j
