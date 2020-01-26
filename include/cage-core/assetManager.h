#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

namespace cage
{
	namespace detail
	{
#ifdef _MSC_VER
		template<class T> GCHL_API_IMPORT char assetClassId;
#else
		template<class T> extern char assetClassId;
#endif // _MSC_VER
	}

	class CAGE_API AssetManager : private Immovable
	{
	public:
		template<class T>
		void defineScheme(uint32 scheme, const AssetScheme &value)
		{
			defineScheme_(scheme, reinterpret_cast<uintPtr>(&detail::assetClassId<T>), value);
		}

		// begin thread-safe methods

		void add(uint32 assetName);
		void remove(uint32 assetName);
		void reload(uint32 assetName);
		uint32 generateUniqueName();

		template<uint32 Scheme, class T>
		void fabricate(uint32 assetName, Holder<T> &&value, const string &textName = "<fabricated>")
		{
			fabricate_(assetName, textName, Scheme, reinterpret_cast<uintPtr>(&detail::assetClassId<T>), templates::move(value).template cast<void>());
		}

		template<uint32 Scheme, class T>
		Holder<T> tryGet(uint32 assetName) const
		{
			return get_(assetName, Scheme, reinterpret_cast<uintPtr>(&detail::assetClassId<T>), false).template cast<T>();
		}

		template<uint32 Scheme, class T>
		[[deprecated]] T *tryGetRaw(uint32 assetName) const
		{
			return tryGet<Scheme, T>(assetName).get();
		}

		template<uint32 Scheme, class T>
		Holder<T> get(uint32 assetName) const
		{
			return get_(assetName, Scheme, reinterpret_cast<uintPtr>(&detail::assetClassId<T>), true).template cast<T>();
		}

		template<uint32 Scheme, class T>
		[[deprecated]] T *getRaw(uint32 assetName) const
		{
			return get<Scheme, T>(assetName).get();
		}

		// end thread-safe methods

		bool processCustomThread(uint32 threadIndex);
		void unloadCustomThread(uint32 threadIndex);
		void unloadWait();
		bool unloaded() const;

		void listen(const string &address, uint16 port);

		EventDispatcher<bool(uint32, MemoryBuffer&)> findAssetBuffer; // this event is called from the loading thread
		EventDispatcher<bool(uint32, string&)> findAssetPath; // this event is called from the loading thread

	private:
		void defineScheme_(uint32 scheme, uintPtr typeId, const AssetScheme &value);
		void fabricate_(uint32 assetName, const string &textName, uint32 scheme, uintPtr typeId, Holder<void> &&value);
		Holder<void> get_(uint32 assetName, uint32 scheme, uintPtr typeId, bool throwOnInvalidScheme) const;
	};

	struct CAGE_API AssetManagerCreateConfig
	{
		string assetsFolderName = "assets.zip";
		uint64 maintenancePeriod = 25000;
		uint64 listenerPeriod = 100000;
		uint32 threadsMaxCount = 5;
		uint32 schemesMaxCount = 50;
	};

	CAGE_API Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config);

	struct CAGE_API AssetPack {};
	CAGE_API AssetScheme genAssetSchemePack();
	static const uint32 AssetSchemeIndexPack = 0;

	CAGE_API AssetScheme genAssetSchemeRaw();
	static const uint32 AssetSchemeIndexRaw = 1;
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
