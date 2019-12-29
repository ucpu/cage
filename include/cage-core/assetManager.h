#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

namespace cage
{
	enum class AssetStateEnum : uint32
	{
		Ready,
		NotFound,
		Error,
		Unknown,
	};

	class CAGE_API AssetManager : private Immovable
	{
	public:
		template<class T>
		void defineScheme(uint32 index, const AssetScheme &value)
		{
			zScheme(index, value, sizeof(T));
		}

		void add(uint32 assetName);
		void fabricate(uint32 scheme, uint32 assetName, const string &textName = "");
		void remove(uint32 assetName);
		void reload(uint32 assetName, bool recursive = false);
		AssetStateEnum state(uint32 assetName) const;
		bool ready(uint32 assetName) const;
		uint32 scheme(uint32 assetName) const;
		uint32 generateUniqueName();

		template<uint32 Scheme, class T>
		T *tryGet(uint32 assetName) const
		{
			if (!ready(assetName))
				return nullptr;
			return get<Scheme, T>(assetName);
		}
		template<uint32 Scheme>
		void *tryGet(uint32 assetName) const
		{
			if (!ready(assetName))
				return nullptr;
			return get<Scheme>(assetName);
		}

		template<uint32 Scheme, class T>
		T *get(uint32 assetName) const
		{
			CAGE_ASSERT(ready(assetName));
			CAGE_ASSERT(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT(zGetTypeSize(Scheme) == sizeof(T), zGetTypeSize(Scheme), sizeof(T), assetName);
			return (T*)zGet(assetName);
		}
		template<uint32 Scheme>
		void *get(uint32 assetName) const
		{
			CAGE_ASSERT(ready(assetName));
			CAGE_ASSERT(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT(zGetTypeSize(Scheme) == m, zGetTypeSize(Scheme), assetName);
			return zGet(assetName);
		}

		template<uint32 Scheme, class T>
		void set(uint32 assetName, T *value)
		{
			CAGE_ASSERT(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT(zGetTypeSize(Scheme) == sizeof(T), zGetTypeSize(Scheme), sizeof(T), assetName);
			zSet(assetName, value);
		}
		template<uint32 Scheme>
		void set(uint32 assetName, void *value)
		{
			CAGE_ASSERT(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT(zGetTypeSize(Scheme) == m, zGetTypeSize(Scheme), assetName);
			zSet(assetName, value);
		}

		uint32 dependenciesCount(uint32 assetName) const;
		uint32 dependencyName(uint32 assetName, uint32 index) const;
		PointerRange<const uint32> dependencies(uint32 assetName) const;

		bool processCustomThread(uint32 threadIndex);
		bool processControlThread();

		uint32 countTotal() const;
		uint32 countProcessing() const;

		void listen(const string &address, uint16 port);

		EventDispatcher<bool(uint32, MemoryBuffer&)> findAssetBuffer; // this event is called from the loading thread
		EventDispatcher<bool(uint32, string&)> findAssetPath; // this event is called from the loading thread

	private:
		void zScheme(uint32 index, const AssetScheme &value, uintPtr typeSize);
		uintPtr zGetTypeSize(uint32 scheme) const;
		void *zGet(uint32 assetName) const;
		void zSet(uint32 assetName, void *value);
	};

	template<>
	inline void AssetManager::defineScheme<void>(uint32 index, const AssetScheme &value)
	{
		zScheme(index, value, m);
	}

	struct CAGE_API AssetManagerCreateConfig
	{
		string assetsFolderName;
		uint32 threadMaxCount;
		uint32 schemeMaxCount;
		AssetManagerCreateConfig();
	};

	CAGE_API Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config);

	CAGE_API AssetScheme genAssetSchemePack(const uint32 threadIndex);
	static const uint32 assetSchemeIndexPack = 0;

	CAGE_API AssetScheme genAssetSchemeRaw(const uint32 threadIndex);
	static const uint32 assetSchemeIndexRaw = 1;
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
