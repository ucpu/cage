#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

#include <cage-core/events.h>
#include <cage-core/typeIndex.h>

namespace cage
{
	class File;

	enum class AssetStateEnum
	{
		None, // asset does not exist
		Loading,
		Ready,
		Error,
	};

	class CAGE_CORE_API AssetsManager : private Immovable
	{
	public:
		template<SchemeIndex Scheme, class T>
		void defineScheme(const AssetsScheme &value)
		{
			defineScheme_(detail::typeHash<T>(), Scheme, value, false);
		}
		template<SchemeIndex Scheme, class T>
		void defineSchemeOverride(const AssetsScheme &value)
		{
			defineScheme_(detail::typeHash<T>(), Scheme, value, true);
		}

		// begin thread-safe methods

		void load(AssetId assetId);
		void unload(AssetId assetId);
		void reload(AssetId assetId);
		void reloadAll();

		AssetId generateUniqueId();

		template<SchemeIndex Scheme, class T>
		void loadValue(AssetId assetId, Holder<T> &&value, const String &textId = "")
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			load_(Scheme, assetId, textId, std::move(value).template cast<void>());
		}

		template<SchemeIndex Scheme, class T>
		void loadCustom(AssetId assetId, const AssetsScheme &customScheme, Holder<void> &&customData, const String &textId = "")
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			load_(Scheme, assetId, textId, customScheme, std::move(customData));
		}

		// returns null if the asset is not yet loaded, failed to load, or has different scheme
		template<SchemeIndex Scheme, class T>
		Holder<T> get(AssetId assetId) const
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			return get1_(Scheme, assetId).template cast<T>();
		}

		// returns null if the asset is not yet loaded, failed to load, or has different type
		template<class T>
		Holder<T> get(AssetId assetId) const
		{
			return get2_(detail::typeHash<T>(), assetId).template cast<T>();
		}

		AssetStateEnum state(AssetId assetId) const;

		// end thread-safe methods

		void listen(const String &address = "localhost", uint16 port = 65042, Time listenerPeriod = Time(100'000));
		bool processCustomThread(uint32 threadIndex);
		void unloadCustomThread(uint32 threadIndex);
		void waitTillEmpty();
		bool processing() const;
		bool empty() const;

		EventDispatcher<bool(AssetId requestId, String &foundName, Holder<File> &foundFile)> findAsset; // this event is called from loading threads

	private:
		void defineScheme_(uint32 typeHash, SchemeIndex scheme, const AssetsScheme &value, bool allowOverride);
		void load_(SchemeIndex scheme, AssetId assetId, const String &textId, Holder<void> &&value);
		void load_(SchemeIndex scheme, AssetId assetId, const String &textId, const AssetsScheme &customScheme, Holder<void> &&customData);
		Holder<void> get1_(SchemeIndex scheme, AssetId assetId) const;
		Holder<void> get2_(uint32 typeHash, AssetId assetId) const;
		uint32 schemeTypeHash_(SchemeIndex scheme) const;
		friend class AssetsOnDemand;
	};

	struct CAGE_CORE_API AssetsManagerCreateConfig
	{
		String assetsFolderName = "assets.carch";
		uint32 diskLoadingThreads = 2;
		uint32 customProcessingThreads = 5;
		uint32 schemesMaxCount = 100; // 0..49 for engine and 50..99 for the game
	};

	CAGE_CORE_API Holder<AssetsManager> newAssetsManager(const AssetsManagerCreateConfig &config);
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
