#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

#include <cage-core/events.h>
#include <cage-core/typeIndex.h>

namespace cage
{
	class File;

	class CAGE_CORE_API AssetManager : private Immovable
	{
	public:
		template<uint32 Scheme, class T>
		void defineScheme(const AssetScheme &value)
		{
			defineScheme_(detail::typeHash<T>(), Scheme, value);
		}

		// begin thread-safe methods

		void load(uint32 assetName);
		void unload(uint32 assetName);
		void reload(uint32 assetName);

		uint32 generateUniqueName();

		template<uint32 Scheme, class T>
		void loadValue(uint32 assetName, Holder<T> &&value, const String &textId = "")
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			load_(Scheme, assetName, textId, std::move(value).template cast<void>());
		}

		template<uint32 Scheme, class T>
		void loadCustom(uint32 assetName, const AssetScheme &customScheme, Holder<void> &&customData, const String &textId = "")
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			load_(Scheme, assetName, textId, customScheme, std::move(customData));
		}

		// returns null if the asset is not yet loaded, failed to load, or has different scheme
		template<uint32 Scheme, class T>
		Holder<T> get(uint32 assetName) const
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			return get_(Scheme, assetName).template cast<T>();
		}

		// end thread-safe methods

		void listen(const String &address = "localhost", uint16 port = 65042, uint64 listenerPeriod = 100000);
		bool processCustomThread(uint32 threadIndex);
		void unloadCustomThread(uint32 threadIndex);
		void waitTillEmpty();
		bool processing() const;
		bool empty() const;

		EventDispatcher<bool(uint32 requestName, String &foundName, Holder<File> &foundFile)> findAsset; // this event is called from loading threads

	private:
		void defineScheme_(uint32 typeHash, uint32 scheme, const AssetScheme &value);
		void load_(uint32 scheme, uint32 assetName, const String &textId, Holder<void> &&value);
		void load_(uint32 scheme, uint32 assetName, const String &textId, const AssetScheme &customScheme, Holder<void> &&customData);
		Holder<void> get_(uint32 scheme, uint32 assetName) const;
		uint32 schemeTypeHash_(uint32 scheme) const;
		friend class AssetOnDemand;
	};

	struct CAGE_CORE_API AssetManagerCreateConfig
	{
		String assetsFolderName = "assets.zip";
		uint32 diskLoadingThreads = 2;
		uint32 customProcessingThreads = 5;
		uint32 schemesMaxCount = 100; // 0..49 for engine and 50..99 for the game
	};

	CAGE_CORE_API Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config);

	struct CAGE_CORE_API AssetPack
	{};
	CAGE_CORE_API AssetScheme genAssetSchemePack();
	constexpr uint32 AssetSchemeIndexPack = 0;

	CAGE_CORE_API AssetScheme genAssetSchemeRaw();
	constexpr uint32 AssetSchemeIndexRaw = 1;
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
