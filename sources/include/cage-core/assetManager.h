#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

#include "events.h"
#include "typeIndex.h"

namespace cage
{
	class CAGE_CORE_API AssetManager : private Immovable
	{
	public:
		template<uint32 Scheme, class T>
		void defineScheme(const AssetScheme &value)
		{
			defineScheme_(detail::typeHash<T>(), Scheme, value);
		}

		// begin thread-safe methods

		void add(uint32 assetName);
		void remove(uint32 assetName);
		void reload(uint32 assetName);
		uint32 generateUniqueName();

		template<uint32 Scheme, class T>
		void fabricate(uint32 assetName, Holder<T> &&value, const String &textName = "<fabricated>")
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			fabricate_(Scheme, assetName, textName, std::move(value).template cast<void>());
		}

		// returns null if the asset is not yet loaded or has different scheme
		template<uint32 Scheme, class T>
		Holder<T> tryGet(uint32 assetName) const
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			return get_(Scheme, assetName, false).template cast<T>();
		}

		// returns null if the asset is not yet loaded
		// throws an exception if the asset has different scheme
		template<uint32 Scheme, class T>
		Holder<T> get(uint32 assetName) const
		{
			CAGE_ASSERT(detail::typeHash<T>() == schemeTypeHash_(Scheme))
			return get_(Scheme, assetName, true).template cast<T>();
		}

		// end thread-safe methods

		bool processCustomThread(uint32 threadIndex);
		void unloadCustomThread(uint32 threadIndex);
		void unloadWait();

		bool processing() const;
		bool unloaded() const;

		void listen(const String &address = "localhost", uint16 port = 65042, uint64 listenerPeriod = 100000);

		EventDispatcher<bool(uint32 name, Holder<File> &file)> findAsset; // this event is called from loading threads

	private:
		void defineScheme_(uint32 typeHash, uint32 scheme, const AssetScheme &value);
		void fabricate_(uint32 scheme, uint32 assetName, const String &textName, Holder<void> &&value);
		Holder<void> get_(uint32 scheme, uint32 assetName, bool throwOnInvalidScheme) const;
		uint32 schemeTypeHash_(uint32 scheme) const;
	};

	struct CAGE_CORE_API AssetManagerCreateConfig
	{
		String assetsFolderName = "assets.zip";
		uint32 diskLoadingThreads = 2;
		uint32 customProcessingThreads = 5;
		uint32 schemesMaxCount = 100; // 0..49 for engine and 50..99 for the game
		sint32 tasksPriority = 100;
	};

	CAGE_CORE_API Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config);

	struct CAGE_CORE_API AssetPack {};
	CAGE_CORE_API AssetScheme genAssetSchemePack();
	constexpr uint32 AssetSchemeIndexPack = 0;

	CAGE_CORE_API AssetScheme genAssetSchemeRaw();
	constexpr uint32 AssetSchemeIndexRaw = 1;
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
