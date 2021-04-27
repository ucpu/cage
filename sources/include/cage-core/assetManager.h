#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

#include "events.h"

namespace cage
{
	namespace privat
	{
		template<class T> inline char assetTypeBlock;
		template<class T> const uintPtr assetTypeId = (uintPtr)&assetTypeBlock<T>;
	}

	class CAGE_CORE_API AssetManager : private Immovable
	{
	public:
		template<uint32 Scheme, class T>
		void defineScheme(const AssetScheme &value)
		{
			defineScheme_(privat::assetTypeId<T>, Scheme, value);
		}

		// begin thread-safe methods

		void add(uint32 assetName);
		void remove(uint32 assetName);
		void reload(uint32 assetName);
		uint32 generateUniqueName();

		template<uint32 Scheme, class T>
		void fabricate(uint32 assetName, Holder<T> &&value, const string &textName = "<fabricated>")
		{
			fabricate_(privat::assetTypeId<T>, Scheme, assetName, textName, templates::move(value).template cast<void>());
		}

		template<uint32 Scheme, class T>
		Holder<T> tryGet(uint32 assetName) const
		{
			return get_(privat::assetTypeId<T>, Scheme, assetName, false).template cast<T>();
		}

		template<uint32 Scheme, class T>
		[[deprecated]] T *tryGetRaw(uint32 assetName) const
		{
			return tryGet<Scheme, T>(assetName).get();
		}

		template<uint32 Scheme, class T>
		Holder<T> get(uint32 assetName) const
		{
			return get_(privat::assetTypeId<T>, Scheme, assetName, true).template cast<T>();
		}

		template<uint32 Scheme, class T>
		[[deprecated]] T *getRaw(uint32 assetName) const
		{
			return get<Scheme, T>(assetName).get();
		}

		// end thread-safe methods

		bool processCustomThread(uint32 threadIndex);
		bool processing() const;

		void unloadCustomThread(uint32 threadIndex);
		void unloadWait();
		bool unloaded() const;

		void listen(const string &address = "localhost", uint16 port = 65042);

		EventDispatcher<bool(uint32 name, Holder<File> &file)> findAsset; // this event is called from the loading thread

	private:
		void defineScheme_(uintPtr typeId, uint32 scheme, const AssetScheme &value);
		void fabricate_(uintPtr typeId, uint32 scheme, uint32 assetName, const string &textName, Holder<void> &&value);
		Holder<void> get_(uintPtr typeId, uint32 scheme, uint32 assetName, bool throwOnInvalidScheme) const;
	};

	struct CAGE_CORE_API AssetManagerCreateConfig
	{
		string assetsFolderName = "assets.zip";
		uint64 listenerPeriod = 100000;
		uint32 threadsMaxCount = 5;
		uint32 schemesMaxCount = 100; // 0..49 for engine and 50..99 for the game
	};

	CAGE_CORE_API Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config);

	struct CAGE_CORE_API AssetPack {};
	CAGE_CORE_API AssetScheme genAssetSchemePack();
	constexpr uint32 AssetSchemeIndexPack = 0;

	CAGE_CORE_API AssetScheme genAssetSchemeRaw();
	constexpr uint32 AssetSchemeIndexRaw = 1;
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
