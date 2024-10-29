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

		// returns null if the asset is not yet loaded or has different scheme
		template<uint32 Scheme, class T>
		Holder<T> get(uint32 assetId, bool autoLoad = true)
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			return get_(Scheme, assetId, autoLoad).template cast<T>();
		}

		void process();
		void clear();

		// end thread-safe methods

	private:
		Holder<void> get_(uint32 scheme, uint32 assetId, bool autoLoad);
		uint32 schemeTypeHash_(uint32 scheme) const;
	};

	CAGE_CORE_API Holder<AssetsOnDemand> newAssetsOnDemand(AssetsManager *assets);
}

#endif // guard_assetsOnDemand_h_wesdxfc74j
